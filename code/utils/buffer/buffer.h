/*
 * @file        : buffer.h
 * @Author      : zhenxi
 * @Date        : 2024-03-11
 * @copyleft    : Apache 2.0
 * Description  : This file contains the declaration of the Buffer class, which is designed for 
 *                managing a memory buffer in a safe and efficient manner. It encapsulates 
 *                operations such as adding data to the buffer, retrieving data, and managing 
 *                the memory to avoid overflows and underflows.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>
#include <cstring>
#include <iostream>
#include <algorithm>

/**
 * @class Buffer
 * @brief The Buffer class is used to manage a dynamic memory buffer.
 * 
 * This class provides functionalities to add data to the buffer, retrieve data from it,
 * and check the current state of the buffer (e.g., current size). It aims to simplify 
 * memory management tasks and ensure data integrity during operations.
 */
class Buffer {
public:
    /**
     * @brief Constructor for Buffer.
     * @param size The initial size of the buffer.
     */
    Buffer(int size = 1024);

    /**
     * @brief Deconstructor for Buffer.
     * Cleans up the allocated memory to avoid memory leaks
     */
    ~Buffer() = default;

    /**
     * @brief A pointer of the buffer data.
     * @return A pointer to the buffer data.
     */
    const char * data() const;

    /**
     * @brief Current valid data size of the buffer
     * @return The data size(number of chars).
     */
    size_t size() const;

    /**
     * @brief Add data to the buffer.
     * @param str The string to be added.
     */
    void addData(const std::string& str);

    /**
     * @brief Add data to the buffer.
     * @param str The pointer to the string.
     * @param len The length of the string.
     */
    void addData(const char* str, size_t len);
        
    /**
     * @brief Add data to the buffer.
     * @param buff The buff to be addDataed.
     */
    void addData(const Buffer& buff);

    /**
     * @brief Delete data in the buffer.
     * @param len The length of data.
     */
    void delData(size_t len);

    /**
     * @brief Get data from the buffer.
     * @param len The length of data.
     * @return The data from the buffer as a string.
     */
    std::string getData(size_t len);

    /**
     * @brief Get string from the buffer which end with the suffix.
     * @param suffix The suffix of string.
     * @return The data from the buffer as a string.
     */
    std::string getUntil(const std::string& suffix);

    /**
     * @brief Read data from the File Descriptor.
     * @param fd The file descriptor of the data to be read.
     * @param Errno The pointer to save the errno.
     * @return The number of chars read from fd.
     */
    ssize_t readFd(int fd, int* Errno);

private:
    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_;   // The position where the valid data starts.
    std::atomic<std::size_t> writePos_;  // The position where the valid data ends.
};

#endif //BUFFER_H