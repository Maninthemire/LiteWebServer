#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024):epollFd_(epoll_create(512)), events_(maxEvent){
        assert(epollFd_ >= 0 && events_.size() > 0);
    }

    ~Epoller(){
        close(epollFd_);
    }

    bool AddFd(int fd, uint32_t events) {
        if(fd < 0) return false;
        epoll_event ev = {0};
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
    }

    bool ModFd(int fd, uint32_t events) {
        if(fd < 0) return false;
        epoll_event ev = {0};
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
    }

    bool DelFd(int fd) {
        if(fd < 0) return false;
        epoll_event ev = {0};
        return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
    }

    int Wait(int timeoutMs) {
        return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
    }

    int GetEventFd(size_t i) const {
        assert(i < events_.size() && i >= 0);
        return events_[i].data.fd;
    }

    uint32_t GetEvents(size_t i) const {
        assert(i < events_.size() && i >= 0);
        return events_[i].events;
    }
        
private:
    int epollFd_;

    std::vector<struct epoll_event> events_;    
};

#endif //EPOLLER_H