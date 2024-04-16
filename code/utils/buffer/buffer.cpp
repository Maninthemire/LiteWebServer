/*
 * @file        : buffer.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-11
 * @copyleft    : Apache 2.0
 */

#include "buffer.h"

Buffer::Buffer(int size) : buffer_(size), readPos_(0), writePos_(0) {}

const char * Buffer::data() const{
    return buffer_.data() + readPos_;
}

size_t Buffer::size() const{
    return writePos_ - readPos_;
}

void Buffer::addData(const char* str, size_t len){
    if(writePos_ + len <= buffer_.size()){
        std::copy(str, str + len, buffer_.data() + writePos_);
    }
    else if(readPos_ + len <= buffer_.size()){
        std::copy(buffer_.data() + readPos_, buffer_.data() + writePos_, buffer_.data());
        writePos_ -= readPos_;
        readPos_ = 0;
        std::copy(str, str + len, buffer_.data() + writePos_);
    }
    else{
        buffer_.resize(writePos_ + len + 1);
        std::copy(str, str + len, buffer_.data() + writePos_);
    }
    writePos_ += len;
}

void Buffer::addData(const std::string& str){
    addData(str.data(), str.size());
}

void Buffer::addData(const Buffer& buff){
    addData(buff.data(), buff.size());
}

int Buffer::strPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int len = vecPrintf(format, args);
    va_end(args);
    return len;
}

int Buffer::vecPrintf(const char *__restrict__ format, va_list &arg) {
    int len = vsnprintf(buffer_.data() + writePos_, buffer_.size() - writePos_, format, arg);
    writePos_ += len;
    return len;
}

void Buffer::delData(size_t len){
    if(len >= size())
        readPos_ = writePos_ = 0;
    else
        readPos_ += len;
}

std::string Buffer::getData(size_t len){
    if(len > size())
        return "";
    std::string str(buffer_.data() + readPos_, len);
    delData(len);
    return str;
}

std::string Buffer::getUntil(const std::string &suffix){
    const char* lineEnd = std::search(data(), data() + size(), suffix.begin(), suffix.end());
    // If not found, return empty string and keep buffer unmodified.
    if(lineEnd == data() + size()){
        std::cout<<buffer_.data()<<std::endl;
        return ""; 
    }
    std::string line(data(), lineEnd + suffix.size());
    delData(line.size());
    return line;
}

ssize_t Buffer::readFd(int fd, int* Errno){
    char buff[65535];
    struct iovec iov_[2];
    iov_[0].iov_base = buffer_.data() + writePos_;
    iov_[0].iov_len = buffer_.size() - writePos_;
    iov_[1].iov_base = buff;
    iov_[1].iov_len = sizeof(buff);
    ssize_t len = readv(fd, iov_, 2);
    if(len <= 0)
        *Errno = errno;
    else if(writePos_ + len <= buffer_.size())
        writePos_ += len;
    else{
        size_t bufferWriteSize = buffer_.size() - writePos_;
        writePos_ = buffer_.size();
        addData(buff, len - bufferWriteSize);
    }
    return len;
}