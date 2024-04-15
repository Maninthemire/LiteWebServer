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

bool HttpRequest::parse(Buffer& buff) {
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
    return state_ >= BODY;
}

inline std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.size()) {
                int value = 0;
                std::istringstream iss(str.substr(i + 1, 2));
                if (iss >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                }
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

bool HttpRequest::parseURL(Buffer& buff) {
    while(buff.size() && state_ != FINISH && state_ != INVALID) {
        std::string pair = buff.getUntil("&");
        if(!pair.size()){
            pair = buff.getUntil(CRLF);
            if(!pair.size())
                return;
            pair = pair.substr(0, pair.length() - 2);            
            state_ = FINISH;
            return true;
        }
        else{
            pair = pair.substr(0, pair.length() - 1);
        }
        std::regex pattern("^([^=]+)=(.+)$");
        std::smatch subMatch;
        if(std::regex_match(pair, subMatch, pattern)) 
            post_[urlDecode(subMatch[1])] = urlDecode(subMatch[2]);
        else
            state_ = INVALID;
    }
    // LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return state_ >= FINISH;
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

std::string HttpRequest::getPost(std::string key) const{
    if(post_.count(key))
        return post_.at(key);
    return "";
};

void HttpRequest::parseRequestLine_(const std::string& line) {
    std::regex pattern("^([^ ]+) ([^ ]+) HTTP/([^ \r]+)\r\n$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, pattern)) {   
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

    std::regex pattern("^([^:]+): ?[^ \r]+)\r\n$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, pattern)) 
        header_[subMatch[1]] = subMatch[2];
    else
        state_ = INVALID;
}