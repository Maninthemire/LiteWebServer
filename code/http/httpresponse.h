/*
 * @file        : httpresponse.h
 * @Author      : zhenxi
 * @Date        : 2024-03-12
 * @copyleft    : Apache 2.0
 * Description  : This file contains the declaration of the HttpResponse class, which is designed for 
 *                making a http response message in a safe and efficient manner. It encapsulates 
 *                operations such as making response message to the buffer and managing resource files.
 */

#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <assert.h> 
#include <string>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <unordered_map>
#include "../utils/buffer/buffer.h"
#include <regex>
#include <errno.h>

/**
 * @class HttpResponse
 * @brief The HttpResponse class is used to making HTTP response messages.
 * 
 * This class provides functionalities to make HTTP request messages 
 * and manage tranferation to the socket. To make it more efficient, the 
 * body content of response messages may remains in the file system.
 */
class HttpResponse {
public:
    /**
     * @brief Constructor for HttpResponse.
     * To initialze the data structures.
     */
    HttpResponse() { clear(); };
    
    /**
     * @brief Deconstructor for HttpResponse.
     * To free the space of its data structure by default.
     */
    ~HttpResponse() { clear(); };
    
    /**
     * @brief To clear the previous response message (if any).
     */
    void clear();

    /**
     * @brief Add a field in the header
     */
    void addHeader(const std::string& key, const std::string& value);
    
    /**
     * @brief Add body of the response message.
     * @param filename A string as the filename.
     * @return A flag whether it succeeds.
     */
    bool addBody(std::string& filename);

    /**
     * @brief To make the response message.
     */
    void makeMessage(Buffer& buff, int code);

private:
    int code_; // Status code
    std::unordered_map<std::string, std::string> header_; // Fields of response header
    bool contentComplete;
    int contentFd;
    off64_t contentLen;
    off64_t contentOffset;


    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    // static const std::unordered_map<int, std::string> CODE_PATH;

    const std::string CRLF = "\r\n"; // Suffix of Carriage Return Line Feed
};

#endif //HTTP_RESPONSE_H