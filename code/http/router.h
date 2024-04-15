/*
 * @file        : router.h
 * @Author      : zhenxi
 * @Date        : 2024-03-12
 * @copyleft    : Apache 2.0
 * Description  : This file contains the declaration of the Router class, which is designed for 
 *                calling corresponding handlers for different methods and urls.
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <assert.h> 
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include "httpconn.h"
#include "../utils/buffer/buffer.h"
#include <string>
#include <regex>
#include <errno.h>
#include <unordered_map>
#include <functional>
#include <string>

/**
 * @class Router
 * @brief The Router class is used to route HTTP requests to the appropriate handlers.
 *
 * This class provides functionalities to map URLs to their respective handler functions
 * and manage the execution of these functions based on the incoming request.
 */
class Router {
public:
    /**
     * @brief Type definition for handler functions.
     */
    using HandlerFunc = std::function<bool(HttpConn&)>;

    /**
     * @brief Constructor for Router.
     * Initializes the Router by loading routes.
     */
    Router() { loadRoutes_(); };

    // /**
    //  * @brief Routes the request to the appropriate handler.
    //  * @param connection The HTTP connection.
    //  * @return True if the request was handled successfully, false otherwise.
    //  */
    // bool routeRequest(HttpConn& connection);

    /**
     * @brief Routes the request to the appropriate handler.
     * @param connection The HTTP connection.
     * @return True if the request was handled successfully, false otherwise.
     */
    HandlerFunc Router::getHandler(HttpConn& connection) ;

private:
    /**
     * @brief Loads the routes into the map from URL to handlers.
     */
    void loadRoutes_();

    // To store the mapping from method and URL to handlers.
    std::unordered_map<std::string, std::unordered_map<std::string, HandlerFunc>> routes;
    
    static std::string srcDir;

    // Utility to add a route to the map
    void addRoute_(const std::string& method, const std::string& url, HandlerFunc handler); 

    inline static void setConnectionHeaders_(HttpConn& connection) {
        if(connection.request_.getHeader("Connection") == "keep-alive" && connection.request_.version() == "1.1") {
            connection.response_.addHeader("Connection", "keep-alive");
            connection.response_.addHeader("keep-alive", "max=6, timeout=120");
        }
        else {
            connection.response_.addHeader("Connection", "close");
        }
    }

    /**
     * @brief Routes the request to the appropriate handler.
     * @param connection The HTTP connection.
     * @return True if the request was handled successfully, false otherwise.
     */
    static bool errorHandler_(HttpConn&);

    /**
     * @brief Verify the user and password in the request.
     * @param connection The HTTP connection.
     * @return True if the request was handled successfully, false otherwise.
     */
    static bool userVerify_(HttpConn& connection);

    /**
     * @brief Create the user with password in the request.
     * @param connection The HTTP connection.
     * @return True if the request was handled successfully, false otherwise.
     */
    static bool userCreate_(HttpConn& connection);

    /**
     * @brief Respond the request with the resource.
     * @param connection The HTTP connection.
     * @param path_to_file The path to the resource file.
     * @return True if the request was handled successfully, false otherwise.
     */
    static bool getResource_(HttpConn& connection, std::string path_to_file);

    static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif //ROUTER_H