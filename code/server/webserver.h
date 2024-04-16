/*
 * @file        : webserver.h
 * @Author      : zhenxi
 * @Date        : 2024-03-15
 * @copyleft    : Apache 2.0
 * Description  : 
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../utils/timer/timer.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/router.h"
#include "../log/log.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, 
        int sqlPort, const char* sqlUser, const  char* sqlPwd, 
        const char* dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();
    void start();

private:
    bool initSocket_(); 
    void initEventMode_(int trigMode);
    void addClient_(int fd, sockaddr_in addr);
  
    void dealListen_();
    void dealWrite_(HttpConn* client);
    void dealRead_(HttpConn* client);

    void sendError_(int fd, const char*info);
    void extentTime_(HttpConn* client);
    void closeConn_(HttpConn* client);

    void onRead_(HttpConn* client);
    void onWrite_(HttpConn* client);
    void onProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    static int setNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;  /* 毫秒MS */
    bool isClose_;
    int listenFd_;
    char* srcDir_;
    
    uint32_t listenEvent_;
    uint32_t connEvent_;
   
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};

#endif //WEBSERVER_H