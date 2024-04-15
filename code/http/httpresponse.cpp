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
    body_ = "";
    header_.clear();
    if(contentFd > 0)
        close(contentFd);
    contentComplete = false;
    contentFd = -1;
    contentLen = contentOffset = 0;
}

void HttpResponse::addHeader(const std::string &key, const std::string &value){
    header_[key] = value;
}

// bool HttpResponse::addBody(std::string &str){
//     if(contentComplete)
//         return false;
//     addHeader("Content-Type", "text/plain");
//     addHeader("Content-Length", std::to_string(str.size()));
//     body_ = str;
//     contentComplete = true;
//     return true;
// }

bool HttpResponse::addBody(std::string &filepath){
    if(contentComplete)
        return false;
    contentFd = open(filepath.data(), O_RDONLY);
    struct stat file_stat;
    if (fstat(contentFd, &file_stat) == -1) 
        return false;
    contentLen = file_stat.st_size;
    contentComplete = true;

    std::string typeName = "text/plain";
    std::string::size_type idx = filepath.find_last_of('.');
    if(idx != std::string::npos) {
        std::string suffix = filepath.substr(idx);
        if(SUFFIX_TYPE.count(suffix) == 1) {
            typeName = SUFFIX_TYPE.find(suffix)->second;
        }
    }
    addHeader("Content-Type", typeName);
    addHeader("Content-Length", std::to_string(contentLen));
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
}