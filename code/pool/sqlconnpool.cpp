/*
 * @file        : webserver.cpp
 * @Author      : zhenxi
 * @Date        : 2024-03-15
 * @copyleft    : Apache 2.0
 * Description  : 
 */

#include "sqlconnpool.h"

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char* host, int port,
            const char* user,const char* pwd, const char* dbName,
            int connSize = 10) {
    // assert(connSize > 0);
    for (int i = 0; i < connSize; i++) {
        MYSQL * init_sql = mysql_init(nullptr);
        if (!init_sql) {
            LOG_ERROR("MySql init error!");
            // assert(init_sql);
        }
        MYSQL * sql = mysql_real_connect(init_sql, host,
                                 user, pwd,
                                 dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql Connect error!");
            std::cout << mysql_error(init_sql) << std::endl;
            // assert(sql);
        }
        connQue_.push(sql);
    }
    sem_init(&semId_, 0, connSize);
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) {
    // assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while(!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    mysql_library_end();        
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}
