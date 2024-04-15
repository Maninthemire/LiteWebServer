/*
 * @file        : timer.h
 * @Author      : zhenxi
 * @Date        : 2024-03-13
 * @copyleft    : Apache 2.0
 */

#include "timer.h"

/**
 * @class Timer
 * @brief The Timer class is used to manage multiple timed tasks.
 * 
 * This class provides functionalities to add, update, and fetch the next 
 * scheduled task. It aims to to efficiently track and handle timed events.
 */

int Timer::timeToExpire(const TimerTask& task){
    return std::chrono::duration_cast<MS>(task.executeTime - Clock::now()).count();
}

/**
 * @class HeapTimer
 * @brief The HeapTimer class use a min-heap structure to manage multiple timed tasks.
 * 
 * This class provides functionalities to add, update, and fetch the next scheduled task. 
 * It utilizes a min-heap structure to efficiently track and handle timed events.
 */

void HeapTimer::swapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
} 

void HeapTimer::siftUp_(size_t index) {
    assert(index < heap_.size());
    size_t i = index;
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        if(heap_[j] < heap_[i]) { break; }
        swapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void HeapTimer::siftDown_(size_t index) {
    assert(index < heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < heap_.size()) {
        if(j + 1 < heap_.size() && heap_[j + 1] < heap_[j]) j++;
        if(heap_[i] < heap_[j]) break;
        swapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
}

void HeapTimer::popNode_(){
    heap_.front().taskFunc();
    swapNode_(0, heap_.size() - 1);
    ref_.erase(heap_.back().id);
    heap_.pop_back();
    if(!heap_.empty())
        siftDown_(0);
}

HeapTimer::~HeapTimer(){
    heap_.clear();
    ref_.clear();
}

void HeapTimer::addTask(const TimerTask& task){
    if(ref_.count(task.id))
        updateTask(task.id, task.executeTime);
    else{
        heap_.push_back(task);
        ref_[task.id] = heap_.size() - 1;
        siftUp_(heap_.size() - 1);
    }
}

void HeapTimer::updateTask(size_t id, const TimeStamp& executeTime){
    assert(ref_.count(id));
    heap_[ref_[id]].executeTime = executeTime;
    siftUp_(ref_[id]);
    siftDown_(ref_[id]);
}

int HeapTimer::nextTick(){
    if(heap_.empty()) 
        return -1;
    
    // Execute tasks that are due.
    while(!heap_.empty() && timeToExpire(heap_.front()) <= 0) 
        popNode_();
    
    // The waiting time (in milliseconds) for the next task
    // -1 for no task, 0 for next task already due
    int timeMS = -1;
    if(!heap_.empty())
        timeMS = timeToExpire(heap_.front()) > 0 ? timeToExpire(heap_.front()) : 0;
    return timeMS;
}