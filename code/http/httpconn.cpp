/*
 * @file        : httpconn.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-13
 * @copyleft    : Apache 2.0
 */

#include "httpconn.h"
#include "router.h"

bool HttpConn::isET;
Router HttpConn::router;

HttpConn::HttpConn() { 
    socketFd_ = -1;
    addr_ = { 0 };
};

HttpConn::~HttpConn() { 
    close(socketFd_);
    LOG_INFO("Client[%d](%s:%d) quit", socketFd_, getIP(), getPort());
};

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    socketFd_ = fd;
    addr_ = addr;
    readBuff_.delData(readBuff_.size());
    writeBuff_.delData(writeBuff_.size());
    LOG_INFO("Client[%d](%s:%d) in", socketFd_, getIP(), getPort());
}

int HttpConn::getFd() const {
    return socketFd_;
};

struct sockaddr_in HttpConn::getAddr() const {
    return addr_;
}

const char* HttpConn::getIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::getPort() const {
    return addr_.sin_port;
}

off64_t HttpConn::toWriteBytes() const {
    return writeBuff_.size() + response_.contentLen() - response_.contentOffset();
}

off64_t HttpConn::readSocket(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.readFd(socketFd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

bool HttpConn::process() {
    if(!readBuff_.size()) {
        LOG_DEBUG("No Data in Buffer");
        return false;
    }
    if(!request_.parse(readBuff_)){
        LOG_DEBUG("Header not Ready");
        return false;        
    }
    if (!cachedHandler) {  // 如果没有缓存的处理函数
        // LOG_DEBUG("Getting Handler");
        cachedHandler = router.getHandler(*this);
    } 
    if(!cachedHandler(*this))   // 如果处理函数还需要等待数据
        return false;
    // 处理函数执行完毕，回收并重置request类解析结果
    cachedHandler = nullptr;
    request_.clear();
    return true;
}

ssize_t HttpConn::writeSocket(int* saveErrno) {
    ssize_t totalLen = 0;
    do {
        if(writeBuff_.size()){
            ssize_t len = write(socketFd_, writeBuff_.data(), writeBuff_.size());
            if(len <= 0) {
                *saveErrno = errno;
                break;
            }
            writeBuff_.delData(len);
            totalLen += len;
        }// Send the header.
        else{   
            off64_t offset = response_.contentOffset();

            size_t count = SSIZE_MAX;
            if(response_.contentLen() < response_.contentOffset() + SSIZE_MAX)
                count = response_.contentLen() - response_.contentOffset();

            ssize_t len = sendfile(socketFd_, response_.contentFd(), &offset, count);
            if(len <= 0) {
                *saveErrno = errno;
                break;
            }
            // LOG_DEBUG("Content Length:%d Offset:%d Send:%d", response_.contentLen(), offset, len);
            response_.contentSend(len);
            totalLen += len;
        }
    } while(isET || toWriteBytes() > 10240);
    return totalLen;
}