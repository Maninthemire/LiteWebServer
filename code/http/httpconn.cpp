/*
 * @file        : httpconn.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-13
 * @copyleft    : Apache 2.0
 */

#include "httpconn.h"

HttpConn::HttpConn() { 
    socketFd_ = -1;
    addr_ = { 0 };
};

HttpConn::~HttpConn() { 
    close(socketFd_);
    // LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
};

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    socketFd_ = fd;
    addr_ = addr;
    readBuff_.delData(readBuff_.size());
    writeBuff_.delData(writeBuff_.size());
    // LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
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
    if(!readBuff_.size()) 
        return false;
    if(!request_.parse(readBuff_))
        return false;
    if (!cachedHandler) {  // 如果没有缓存的处理函数
        cachedHandler = router.getHandler(*this);
    } 
    if(!cachedHandler(*this))
        return false;
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
            off64_t count = response_.contentLen() - response_.contentOffset();
            ssize_t len = sendfile(socketFd_, response_.contentFd(), &offset, count);
            if(len <= 0) {
                *saveErrno = errno;
                break;
            }
            response_.contentSend(len);
            totalLen += len;
        }
    } while(isET || toWriteBytes() > 10240);
    return totalLen;
}