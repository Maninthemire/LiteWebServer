/*
 * @file        : httpconn.h
 * @Author      : zhenxi
 * @Date        : 2024-03-13
 * @copyleft    : Apache 2.0
 * Description  : This file contains the declaration of the HttpConn class, which is designed 
 *                for managing an HTTP connection. It uses the Buffer class as a buffer for IO 
 *                with sockets and employs the HttpConn class and HttpResponse class to parse 
 *                and construct HTTP messages. It encapsulates methods for reading, writing, 
 *                processing, and responding to HTTP requests.
 */

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      
#include <unordered_set>
#include "../pool/sqlconnRAII.h"
#include "../utils/buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
// #include "../log/log.h"

class Router;
/**
 * @class HttpConn
 * @brief The HttpConn class is used to manage HTTP connections.
 * 
 * This class provides functionalities to manage the IO of HTTP 
 * connections.
 */
class HttpConn {
    friend class Router;
public:
    /**
     * @brief Constructor for HttpConn.
     * To initialze the data structures.
     */
    HttpConn();
    
    /**
     * @brief Deconstructor for HttpConn.
     * To free the space of its data structure by default.
     */
    ~HttpConn();

    /**
     * @brief  To initialze the HTTP connection.
     * @param fd The File Descriptor of socket.
     * @param addr The address of the connection.
     */
    void init(int fd, const sockaddr_in &addr);

    /**
     * @brief  To get the socket File Descriptor of HTTP connection.
     */
    int getFd() const;

    /**
     * @brief  To get the address of HTTP connection.
     */
    struct sockaddr_in HttpConn::getAddr() const ;

    /**
     * @brief  To get the IP address of HTTP connection.
     */
    const char* HttpConn::getIP() const ;

    /**
     * @brief  To get the port of HTTP connection.
     */
    int HttpConn::getPort() const;

    /**
     * @brief  To read from the socket.
     */
    ssize_t read(int * saveErrno);

    /**
     * @brief  To respond to the request.
     */
    bool process();

    /**
     * @brief  To write into the socket.
     */
    ssize_t write(int *saveErrno);

    static bool isET;
    static const char *srcDir;

private:
    int socketFd_;
    struct  sockaddr_in addr_;

    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);

    /**
     * @brief  To parse the body of request message.
     */
    bool parseBody_();

    /**
     * @brief  To make response to the request message.
     */
    bool makeResponse_();
};

#endif //HTTP_CONN_H