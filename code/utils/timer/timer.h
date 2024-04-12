/*
 * @file        : timer.h
 * @Author      : zhenxi
 * @Date        : 2024-03-13
 * @copyleft    : Apache 2.0
 * Description  : This file defines the Timer interface for scheduling and managing timed tasks 
 *                in applications. It provides a standard way to add, update, and execute tasks 
 *                at specified times, suitable for use in systems that require timed operations.
 */

#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <functional> 
#include <chrono>
#include <assert.h> 

typedef std::chrono::milliseconds MS;
typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimeStamp;

/**
 * @struct TimerTask
 * @brief The TimerTask struct is used to manage a timed task.
 * 
 * It provides a standard way to add, update, and execute tasks at 
 * specified times, suitable for use in systems that require timed operations.
 */
struct TimerTask{
    size_t id;  // For a connection timeout task, id can be the File Descriptor.
    TimeStamp executeTime;
    std::function<void()> taskFunc;
    bool operator<(const TimerTask& t) {
        return executeTime < t.executeTime;
    }
};

/**
 * @class Timer
 * @brief The Timer class is used to manage multiple timed tasks.
 * 
 * This class provides functionalities to add, update, and fetch the next 
 * scheduled task. It aims to to efficiently track and handle timed events.
 */
class Timer {
public:
    /**
     * @brief Deconstructor for Timer.
     * To free the space of its data structure.
     */
    virtual ~Timer() {};

    /**
     * @brief Add a new task into Timer.
     * @param task The new task to be added.
     */
    virtual void addTask(const TimerTask& task) = 0;

    /**
     * @brief Update the task of corresponding id in Timer.
     * @param id The id of the task.
     * @param executeTime The new executeTime of the task.
     */
    virtual void updateTask(size_t id, const TimeStamp& executeTime) = 0;

    /**
     * @brief Execute tasks that are due and return the waiting time for the next task.
     * @return The waiting time (in milliseconds) for the next task.
     */
    virtual int nextTick() = 0;
    
    /**
     * @brief Check if the task has expired.
     * @param task The task being checked.
     * @return The flag whether the task has expired.
     */
    bool hasExpired(const TimerTask& task);
};

/**
 * @class HeapTimer
 * @brief The HeapTimer class use a min-heap structure to manage multiple timed tasks.
 * 
 * This class provides functionalities to add, update, and fetch the next scheduled task. 
 * It utilizes a min-heap structure to efficiently track and handle timed events.
 */
class HeapTimer : public Timer {
public:
    /**
     * @brief Constructor for HeapTimer.
     * To initialize the heap and referrence map.
     */
    HeapTimer() { heap_.reserve(64); };

    /**
     * @brief Deconstructor for HeapTimer.
     * To free the vector and unordered map.
     */
    ~HeapTimer();

    /**
     * @brief Add a new task into Timer.
     * @param task The new task to be added.
     */
    void addTask(const TimerTask& task) override;

    /**
     * @brief Update the task of corresponding id in Timer.
     * @param id The id of the task.
     * @param executeTime The new executeTime of the task.
     */
    void updateTask(size_t id, const TimeStamp& executeTime) override;

    /**
     * @brief Execute tasks that are due and return the waiting time for the next task.
     * @return The waiting time (in milliseconds) for the next task.
     */
    int nextTick() override;

private:
    std::vector<TimerTask> heap_;  
    std::unordered_map<int, size_t> ref_; // Map the task id to the vector index.

    /**
     * @brief Swap nodes in HeapTimer.
     * To swap nodes in the vector and update their referrences.
     */
    void swapNode_(size_t i, size_t j);
    
    /**
     * @brief Sift node up in the heap.
     * @param index The index of the node in the vector.
     * To sift up a node until its parent node(if any) is smaller.
     */
    void siftUp_(size_t index);

    /**
     * @brief Sift node down in the heap.
     * @param index The index of the node in the vector.
     * To sift down a node until both of its child node(if any) is bigger.
     */
    void siftDown_(size_t index);

    /**
     * @brief Pop the top node.
     * Pop out the next task.
     */
    void popNode_();
};

/**
 * @class HashedWheelTimer
 * @brief The HashedWheelTimer class use a hashed wheel structure to manage multiple timed tasks.
 * 
 * This class provides functionalities to add, update, and fetch the next slot's scheduled tasks. 
 * It utilizes a hashed wheel structure to efficiently track and handle timed events.
 */
class HashedWheelTimer : public Timer {
public:
    /**
     * @brief Deconstructor for HeapTimer.
     * To free the space of its data structure.
     */
    ~HashedWheelTimer();

    /**
     * @brief Add a new task into Timer.
     * @param task The new task to be added.
     */
    void addTask(const TimerTask& task) override;

    /**
     * @brief Update the task of corresponding id in Timer.
     * @param id The id of the task.
     * @param executeTime The new executeTime of the task.
     */
    void updateTask(size_t id, const TimeStamp& executeTime) override;

    /**
     * @brief Execute tasks that are due and return the waiting time for the next task.
     * @return The waiting time (in milliseconds) for the next task.
     */
    int nextTick() override;
};

#endif  //TIMER_H