/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mutex>
#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <semaphore.h>
#include <thread>
#include <mutex>
#include <sys/time.h>
#include <iostream>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h> 
#include "../log/log.h"

class SqlConnPool {
    friend class SqlConnRAII;
public:
    static SqlConnPool *Instance();

    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool() = default;
    ~SqlConnPool();

    MYSQL *GetConn();

    void FreeConn(MYSQL * conn);

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;

    // int MAX_CONN_;
    // int useCount_;
    // int freeCount_;
};


#endif // SQLCONNPOOL_H