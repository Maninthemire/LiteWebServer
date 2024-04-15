/*
 * @file        : router.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-12
 * @copyleft    : Apache 2.0
 */

#include "router.h"


const std::unordered_map<int, std::string> Router::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

bool Router::getResource_(HttpConn& connection, std::string path_to_file){
    setConnectionHeaders_(connection);
    connection.response_.makeMessage(connection.writeBuff_, 200);
    std::string fileName = srcDir;
    fileName += path_to_file;
    connection.response_.addBody(fileName);
    return true; // 请求处理完成
}

void HttpRequest::ParseFromUrlencoded_() {
    if(body_.size() == 0) { return; }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        /* 注册行为 且 用户名未被使用*/
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } 
        else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}

bool Router::userVerify_(HttpConn& connection){
    std::string line = connection.readBuff_.getUntil(connection.request_.CRLF);

    if (name.empty() || pwd.empty()) return false;

    // LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    char order[256];
    snprintf(order, 256, "SELECT password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    // LOG_DEBUG("%s", order);

    if (mysql_query(sql, order)) {
        // LOG_DEBUG("Query error!");
        return false;
    }

    MYSQL_RES* res = mysql_store_result(sql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        // LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        flag = pwd == password;
        // if (!flag) 
        //     LOG_DEBUG("Password error!");
    } 
    else {
        // LOG_DEBUG("User not found!");
        flag = false;
    }

    mysql_free_result(res);
    SqlConnPool::Instance()->FreeConn(sql);
    // LOG_DEBUG( "UserVerify success!!");
    if(flag)
        getResource_(connection, "/welcome.html");
    else
        getResource_(connection, "/error.html");
    return true;
}

bool Router::getResource_(HttpConn& connection, std::string path_to_file){
    setConnectionHeaders_(connection);
    connection.response_.makeMessage(connection.writeBuff_, 200);
    std::string fileName = srcDir;
    fileName += path_to_file;
    connection.response_.addBody(fileName);
    return true; // 请求处理完成
}

void Router::addRoute_(const std::string& method, const std::string& url, HandlerFunc handler) {
    routes[method][url] = handler;
}

void Router::loadRoutes_() {
    addRoute_("GET", "/", std::bind(&Router::getResource_, std::placeholders::_1, "/index.html"));
    addRoute_("GET", "/index", std::bind(&Router::getResource_, std::placeholders::_1, "/index.html"));
    addRoute_("GET", "/register", std::bind(&Router::getResource_, std::placeholders::_1, "/register.html"));
    addRoute_("GET", "/login", std::bind(&Router::getResource_, std::placeholders::_1, "/login.html"));
    addRoute_("GET", "/welcome", std::bind(&Router::getResource_, std::placeholders::_1, "/welcome.html"));
    addRoute_("GET", "/video", std::bind(&Router::getResource_, std::placeholders::_1, "/video.html"));
    addRoute_("GET", "/picture", std::bind(&Router::getResource_, std::placeholders::_1, "/picture.html"));

    addRoute_("POST", "/register", std::bind(&Router::getResource_, std::placeholders::_1, "/register.html"));
    addRoute_("POST", "/login", std::bind(&Router::getResource_, std::placeholders::_1, "/login.html"));
}

bool Router::errorHandler_(HttpConn& connection) {
    // Try to GET an unknown URL
    setConnectionHeaders_(connection);
    connection.response_.makeMessage(connection.writeBuff_, 404);
    std::string fileName = srcDir;
    fileName += "/404.html";
    connection.response_.addBody(fileName);
    return true;
}


bool Router::routeRequest(HttpConn& connection) {
    auto methodIt = routes.find(connection.request_.method());
    if (methodIt != routes.end()) {
        auto& pathMap = methodIt->second;
        auto pathIt = pathMap.find(connection.request_.url());
        if (pathIt != pathMap.end()) {
            return pathIt->second(connection);
        }
    }
    return errorHandler_(connection);
}

