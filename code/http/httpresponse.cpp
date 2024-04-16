/*
 * @file        : httpresponse.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-12
 * @copyleft    : Apache 2.0
 */

#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

void HttpResponse::clear() {
    code_ = -1;
    header_.clear();
    if(contentFd_ > 0)
        close(contentFd_);
    contentComplete_ = false;
    contentFd_ = -1;
    contentLen_ = contentOffset_ = 0;
}

void HttpResponse::addHeader(const std::string &key, const std::string &value){
    header_[key] = value;
}

// bool HttpResponse::addBody(std::string &str){
//     if(contentComplete_)
//         return false;
//     addHeader("Content-Type", "text/plain");
//     addHeader("Content-Length", std::to_string(str.size()));
//     body_ = str;
//     contentComplete_ = true;
//     return true;
// }

bool HttpResponse::addBody(std::string &filepath){
    if(contentComplete_){
        LOG_DEBUG("Multiple Content");
        return false;
    }
    contentFd_ = open(filepath.data(), O_RDONLY);
    // std::cout<<contentFd_<<std::endl;
    if (contentFd_ == -1) {
        LOG_DEBUG("Invalid Filepath");
        return false;
    }
    struct stat file_stat;
    if (fstat(contentFd_, &file_stat) == -1) {
        LOG_DEBUG("Error Filelength");
        return false;
    }
    contentLen_ = file_stat.st_size;
    // std::cout<<contentLen_<<std::endl;
    contentComplete_ = true;

    std::string typeName = "text/plain";
    std::string::size_type idx = filepath.find_last_of('.');
    if(idx != std::string::npos) {
        std::string suffix = filepath.substr(idx);
        if(SUFFIX_TYPE.count(suffix) == 1) {
            typeName = SUFFIX_TYPE.find(suffix)->second;
        }
    }
    addHeader("Content-Type", typeName);
    addHeader("Content-Length", std::to_string(contentLen_));
    return true;
}

void HttpResponse::makeMessage(Buffer &buff, int code)
{
    // make status line
    code_ = code;
    std::string status;
    if(CODE_STATUS.count(code_) == 1)
        status = CODE_STATUS.find(code_)->second;
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.addData("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");

    // make response headers
    for(auto field: header_)
        buff.addData(field.first + ": " + field.second + "\r\n");

    LOG_DEBUG("Response Header\n%s", buff.data());
}

int HttpResponse::contentFd() const{
    return contentFd_;
}

off64_t HttpResponse::contentLen() const{
    return contentLen_;
}

off64_t HttpResponse::contentOffset() const{
    return contentOffset_;
}


void HttpResponse::contentSend(off64_t len){
    contentOffset_ += len;
}