/*
 * @file        : router.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-12
 * @copyleft    : Apache 2.0
 */

#include "router.h"

std::string Router::srcDir;

const std::unordered_map<int, std::string> Router::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

bool Router::getResource_(HttpConn& connection, std::string path_to_file){
    setConnectionHeaders_(connection);
    std::string fileName = srcDir;
    fileName += path_to_file;
    LOG_DEBUG("Add Body:%s", fileName.c_str());
    connection.response_.addBody(fileName);
    connection.response_.makeMessage(connection.writeBuff_, 200);
    return true; 
}

bool Router::userVerify_(HttpConn& connection){
    if(!connection.request_.parseURL(connection.readBuff_))
        return false;
    std::string name = connection.request_.getPost("username");
    std::string pwd = connection.request_.getPost("password");
    if (name.empty() || pwd.empty()){
        return getResource_(connection, "error.html");
    }

    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    char order[256];
    snprintf(order, 256, "SELECT password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order)) {
        LOG_DEBUG("Query error!");
        return false;
    }

    MYSQL_RES* res = mysql_store_result(sql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        flag = pwd == password;
        if (!flag) 
            LOG_DEBUG("Password error!");
    } 
    else {
        LOG_DEBUG("User not found!");
        flag = false;
    }

    mysql_free_result(res);
    // SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    if(flag)
        return getResource_(connection, "welcome.html");
    else
        return getResource_(connection, "error.html");
}

bool Router::userCreate_(HttpConn& connection){
    if(!connection.request_.parseURL(connection.readBuff_))
        return false;
    std::string name = connection.request_.getPost("username");
    std::string pwd = connection.request_.getPost("password");
    if (name.empty() || pwd.empty()){
        return getResource_(connection, "error.html");
    }

    LOG_INFO("Create name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = true;
    char order[256];
    snprintf(order, 256, "SELECT password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order)) {
        LOG_DEBUG("Query error!");
        return false;
    }

    MYSQL_RES* res = mysql_store_result(sql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        flag = false;
        LOG_DEBUG("Username used!");
    } 
    mysql_free_result(res);

    LOG_DEBUG("regirster!");
    bzero(order, 256);
    snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
    LOG_DEBUG( "%s", order);
    if(mysql_query(sql, order)) { 
        LOG_DEBUG( "Insert error!");
        flag = false; 
    }

    // SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    if(flag)
        getResource_(connection, "welcome.html");
    else
        getResource_(connection, "error.html");
    return true;
}

void Router::addRoute_(const std::string& method, const std::string& url, HandlerFunc handler) {
    routes[method][url] = handler;
}

void Router::loadRoutes_() {
    addRoute_("GET", "/", std::bind(&Router::getResource_, std::placeholders::_1, "index.html"));
    addRoute_("GET", "/index", std::bind(&Router::getResource_, std::placeholders::_1, "index.html"));
    addRoute_("GET", "/register", std::bind(&Router::getResource_, std::placeholders::_1, "register.html"));
    addRoute_("GET", "/login", std::bind(&Router::getResource_, std::placeholders::_1, "login.html"));
    addRoute_("GET", "/welcome", std::bind(&Router::getResource_, std::placeholders::_1, "welcome.html"));
    addRoute_("GET", "/video", std::bind(&Router::getResource_, std::placeholders::_1, "video.html"));
    addRoute_("GET", "/picture", std::bind(&Router::getResource_, std::placeholders::_1, "picture.html"));

    addRoute_("POST", "/register", std::bind(&Router::getResource_, std::placeholders::_1, "register.html"));
    addRoute_("POST", "/login", std::bind(&Router::getResource_, std::placeholders::_1, "login.html"));
}

bool Router::errorHandler_(HttpConn& connection) {
    // Try to GET an unknown URL
    setConnectionHeaders_(connection);
    std::string fileName = srcDir;
    fileName += "404.html";
    connection.response_.addBody(fileName);
    connection.response_.makeMessage(connection.writeBuff_, 404);
    return true;
}

// bool Router::routeRequest(HttpConn& connection) {
//     auto methodIt = routes.find(connection.request_.method());
//     if (methodIt != routes.end()) {
//         auto& pathMap = methodIt->second;
//         auto pathIt = pathMap.find(connection.request_.url());
//         if (pathIt != pathMap.end()) {
//             return pathIt->second(connection);
//         }
//     }
//     return errorHandler_(connection);
// }

std::function<bool(HttpConn&)> Router::getHandler(HttpConn& connection) {
    auto methodIt = routes.find(connection.request_.method());
    if (methodIt != routes.end()) {
        auto& pathMap = methodIt->second;
        auto pathIt = pathMap.find(connection.request_.url());
        if (pathIt != pathMap.end()) {
            return pathIt->second;
        }
        else if(connection.request_.method() == "GET"){
            // Handler not found, check the resource
            std::string fileName = srcDir;
            fileName += connection.request_.url();
            struct stat filestat;
            if(stat(fileName.c_str(), &filestat) == 0)     // File Exists               
               return std::bind(&Router::getResource_, std::placeholders::_1, connection.request_.url());
        }
    }
    return &errorHandler_;
}