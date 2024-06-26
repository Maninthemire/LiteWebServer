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
        if(!line.size()) {
            // LOG_DEBUG("No Line in Buffer");
            return false;
        }
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
    if(header_.count("Content-Length") == 0){
        // Can't parse unknown length url
        state_ = INVALID;
        return true;
    }

    while(buff.size() && state_ != FINISH && state_ != INVALID) {
        std::string pair = buff.getUntil("&");
        if(!pair.size()){
            // check if its the last kv pair
            if(buff.size() < contentExpect)
                return false;
            pair = buff.getData(contentExpect); 
            contentExpect = 0;           
            state_ = FINISH;
        }
        else{
            contentExpect -= pair.size();      
            pair = pair.substr(0, pair.length() - 1);
        }
        std::regex pattern("^([^=]+)=(.+)$");
        std::smatch subMatch;
        if(std::regex_match(pair, subMatch, pattern)) 
            post_[urlDecode(subMatch[1])] = urlDecode(subMatch[2]);
        else
            state_ = INVALID;
    }
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
    // LOG_DEBUG("Parsing Request Line:%s", line.c_str());
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
    // LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), url_.c_str(), version_.c_str());
}

void HttpRequest::parseHeader_(const std::string& line) {
    // LOG_DEBUG("Parsing Request Header [%s]", line.c_str());
    if(line == "\r\n"){
        if(header_.count("Content-Length"))
            contentExpect = std::stoul(header_["Content-Length"]);
        state_ = BODY;
        return;
    } // Empty line means the header is over.

    std::regex pattern("^([^:]+): ?([^\r]+)\r\n$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, pattern)) {
        header_[subMatch[1]] = subMatch[2];
        // LOG_DEBUG("Header [%s]: [%s]", subMatch[1].str().c_str(), subMatch[2].str().c_str());
    }
    else{
        std::string result;
        for (char c : line) {
            if (c == '\r') {
                result += "\\r";
            } else if (c == '\n') {
                result += "\\n";
            } else if (c == ' ') {
                result += "\\t";
            } else {
                result += c;
            }
        }
        state_ = INVALID;
        LOG_ERROR("Invalid Header:[%s]", result.c_str());
    }
}