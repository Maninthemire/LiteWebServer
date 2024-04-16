/*
 * @file        : webserver.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-15
 * @copyleft    : Apache 2.0
 * Description  : 
 */

#include "webserver.h"

using namespace std;

WebServer::WebServer(
            int port, int trigMode, int timeoutMS, bool OptLinger,
            int sqlPort, const char* sqlUser, const  char* sqlPwd,
            const char* dbName, int connPoolNum, int threadNum,
            bool openLog, int logLevel, int logQueSize):
            port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
    {
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources", 16);
    Router::srcDir = std::string(srcDir_);
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    initEventMode_(trigMode);
    if(!initSocket_()) { isClose_ = true;}

    if(openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            // cout<<Router::srcDir<<endl;
            LOG_INFO("srcDir: %s", Router::srcDir.data());
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    // if(!isClose_) { LOG_INFO("========== Server start =========="); }
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = timer_->nextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) {
                dealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                closeConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                dealRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                dealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::sendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::closeConn_(HttpConn* client) {
    assert(client);
    // LOG_INFO("Client[%d] quit, userCount:%d", client->getFd(), users_.size());
    epoller_->DelFd(client->getFd());
    users_.erase(client->getFd());
}

void WebServer::addClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeoutMS_ > 0) {
        TimeStamp expireTime = Clock::now() + MS(timeoutMS_);
        TimerTask httpConnExpire = {fd, expireTime, std::bind(&WebServer::closeConn_, this, &users_[fd])};
        timer_->addTask(httpConnExpire);
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    setNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].getFd());
}

void WebServer::dealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(users_.size() >= MAX_FD) {
            sendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        addClient_(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::dealRead_(HttpConn* client) {
    assert(client);
    extentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::onRead_, this, client));
}

void WebServer::dealWrite_(HttpConn* client) {
    assert(client);
    extentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::onWrite_, this, client));
}

void WebServer::extentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { 
        TimeStamp expireTime = Clock::now() + MS(timeoutMS_);
        timer_->updateTask(client->getFd(), expireTime); 
    }
}

void WebServer::onRead_(HttpConn* client) {
    assert(client);
    // LOG_DEBUG("Reading From Client[%d]", client->getFd());
    int ret = -1;
    int readErrno = 0;
    ret = client->readSocket(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        LOG_DEBUG("No Data in Socket");
        closeConn_(client);
        return;
    }
    // else
    //     std::cout<<"read "<<ret<<" of chars from socket"<<std::endl;
    onProcess(client);
}

void WebServer::onProcess(HttpConn* client) {
    if(client->process()) {
        // LOG_DEBUG("Waiting for Writing");
        epoller_->ModFd(client->getFd(), connEvent_ | EPOLLOUT);
    } else {
        // LOG_DEBUG("Waiting for Reading");
        epoller_->ModFd(client->getFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HttpConn* client) {
    assert(client);
    LOG_DEBUG("Writing To Client[%d]", client->getFd());
    int ret = -1;
    int writeErrno = 0;
    ret = client->writeSocket(&writeErrno);
    if(client->toWriteBytes() == 0) {
        /* 传输完成 */
        // LOG_DEBUG("Writing To Client[%d] Finished", client->getFd());
        if(client->isKeepAlive()) {
            onProcess(client);
            return;
        }
        else{
            closeConn_(client);
            return;
        }
    }
    else if(ret < 0 && writeErrno != EAGAIN) {
        // LOG_ERROR("Remaining %d bytes, Sending Failed!", client->toWriteBytes());
        closeConn_(client);
        return;
    }
    /* 继续传输 */
    // LOG_DEBUG("Remaining %d bytes, Continue Sending", client->toWriteBytes());
    epoller_->ModFd(client->getFd(), connEvent_ | EPOLLOUT);
    return;
}

/* Create listenFd */
bool WebServer::initSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    setNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

int WebServer::setNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


