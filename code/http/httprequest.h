/*
 * @file        : httprequest.h
 * @Author      : zhenxi
 * @Date        : 2024-03-12
 * @copyleft    : Apache 2.0
 * Description  : This file contains the declaration of the HttpRequest class, which is designed for 
 *                parsing a HTTP request message in a safe and efficient manner. It encapsulates 
 *                operations such as parsing message from the buffer, retrieving headers and fields.
 */

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <assert.h> 
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include "../utils/buffer/buffer.h"

/**
 * @class HttpRequest
 * @brief The HttpRequest class is used to parsing HTTP request messages.
 * 
 * This class provides functionalities to parse HTTP request messages 
 * from Buffer and access the corresponding headers and fields. To make 
 * it more efficient, the content of POST request messages (if any) remains 
 * in the Buffer.
 */
class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,      
        FINISH,
        INVALID,
    };

    /**
     * @brief Constructor for HttpRequest.
     * To initialze the data structures.
     */
    HttpRequest() { clear(); };
    
    /**
     * @brief Deconstructor for HttpRequest.
     * To free the space of its data structure by default.
     */
    ~HttpRequest() = default;

    /**
     * @brief To clear the previous request message (if any).
     */
    void clear();

    /**
     * @brief Parses the incoming HTTP data stored in the buffer.
     * To parse the input as a HTTP request while remains the body 
     * (if any) in the buff.
     * @return True if requet line and header is ready.
     */
    bool parse(Buffer& buff);

    /**
     * @brief Parses the the body as url key-value pairs.
     * @return True if the body is totally parsed.
     */
    bool parseURL(Buffer &buff);

    /**
     * @brief Get the method of request.
     */
    std::string method() const;

    /**
     * @brief Get the URL of request.
     */
    std::string url() const;

    /**
     * @brief Get the http version of request.
     */
    std::string version() const;

    /**
     * @brief Get the value of the field in the header.
     * @return The value of corresponding field (empty if no such field.)
     */
    std::string getHeader(std::string fieldName) const;

    /**
     * @brief Get the value of the key in the url posted.
     * @return The value of corresponding key (empty if no such key.)
     */
    std::string getPost(std::string key) const;

private:
    PARSE_STATE state_;
    std::string method_, url_, version_; // Content of request line
    std::unordered_map<std::string, std::string> header_; // Fields of request header
    std::unordered_map<std::string, std::string> post_; // Content of post request
    const std::string CRLF = "\r\n"; // Suffix of Carriage Return Line Feed
    static const std::unordered_set<std::string> DEFAULT_HTML;
    

    /**
     * @brief Parse the request line.
     * @param line The request line.
     */
    void parseRequestLine_(const std::string& line);

    /**
     * @brief Parse the request header.
     * @param line The request header.
     */
    void parseHeader_(const std::string& line);
};


#endif //HTTP_REQUEST_H