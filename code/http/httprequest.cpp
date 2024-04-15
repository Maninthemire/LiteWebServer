/*
 * @file        : httprequest.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-12
 * @copyleft    : Apache 2.0
 */

#include "httprequest.h"

void HttpRequest::clear() {
    method_ = url_ = version_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
}

void HttpRequest::parse(Buffer& buff) {
    if(!buff.size()) 
        return;
    while(buff.size() && state_ != BODY && state_ != INVALID) {
        std::string line = buff.getUntil(CRLF);
        if(!line.size()) 
            return;
        switch(state_){
            case REQUEST_LINE:
                parseRequestLine_(line);
                break;
            case HEADERS:
                parseHeader_(line);
                break;
            default:
                break;
        }
    }
    // LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
}

bool HttpRequest::headerReady() const{
    return state_ == BODY;
}

std::string HttpRequest::method() const{
    return method_;
}

std::string HttpRequest::url() const{
    return url_;
}

std::string HttpRequest::version() const
{
    return version_;
}

std::string HttpRequest::getHeader(std::string fieldName) const{
    if(header_.count(fieldName))
        return header_.at(fieldName);
    return "";
};

void HttpRequest::parseRequestLine_(const std::string& line) {
    std::regex patten("^([^ ]+) ([^ ]+) HTTP/([^ \r]+)\r\n$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        url_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
    }
    else
        state_ = INVALID;
}

void HttpRequest::parseHeader_(const std::string& line) {
    if(line == "\r\n"){
        state_ = BODY;
        return;
    } // Empty line means the header is over.

    std::regex patten("^([^:]+): ?[^ \r]+)\r\n$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) 
        header_[subMatch[1]] = subMatch[2];
    else
        state_ = INVALID;
}