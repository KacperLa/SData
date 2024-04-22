#ifndef SDATA_HPP_INCLUDED
#define SDATA_HPP_INCLUDED

#include <string>
#include <errno.h>
#include <climits>

#include <string>
#include <cstring>
#include <iostream>
#include <atomic>
#include <chrono>
#include "fcntl.h"
#include "unistd.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace sdata 
{

template<typename T>
class SData
{
public:

    using stype = T;
    using stype_ptr_t = T *;

    SData(const std::string& mapped_file, int timeout, bool isProducer) : 
        mapped_file(mapped_file),
        memory_mapped(false),
        timeout({0, timeout}),
        isProducer(isProducer)
    {
        if (openMap()) {
            log("Error opening mmap");
            return;
        }
        else
        {
            log("Memory mapped successfully");
            memory_mapped = true;
        }
    }

    ~SData()
    {
        closeMap();
    }

    bool isMemoryMapped()
    {
        return memory_mapped.load(std::memory_order_acquire);
    }

    bool getData(stype_ptr_t data)
    {
        int current_buffer_index = ((shared_data->producer_futex.load(std::memory_order_acquire) - 1) % 3);
        std::memcpy(data, 
                    &shared_data->triple_buffer[current_buffer_index],
                    sizeof(T));
        if (current_buffer_index == (shared_data->producer_futex.load(std::memory_order_acquire) % 3))
        {
            log("Buffer index potential corruption");
            return false;
        }
        else
        {
            return true;
        }
    }

    stype_ptr_t getBuffer()
    {
        return &shared_data->triple_buffer[shared_data->producer_futex.load(std::memory_order_acquire) % 3];
    }

    int getBufferIndex() {
        return shared_data->producer_futex.load(std::memory_order_acquire) % 3;
    }

    void trigger()
    {
        shared_data->producer_futex.fetch_add(1, std::memory_order_release);
        futexWakeAll(&shared_data->producer_futex);
    }


    int waitOnStateChange(stype_ptr_t data)
    {   
        // return codes:
        // 0: success
        // 1: timeout
        // 2: potential data corruption

        int current_index = shared_data->producer_futex.load(std::memory_order_acquire);
        if (futexWait(&shared_data->producer_futex, current_index))
        {
            std::memcpy(data,
                        &shared_data->triple_buffer[current_index % 3],
                        sizeof(T));
            if ((current_index % 3) == (shared_data->producer_futex.load(std::memory_order_acquire) % 3))
            {
                logError("Buffer index potential corruption");
                return 2;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            logError("Futex timeout occured");
            return 1;
        }
    }

    void setData(const stype_ptr_t data)
    {
        std::memcpy(&shared_data->triple_buffer[shared_data->producer_futex.load(std::memory_order_acquire)  % 3], 
                    data, 
                    sizeof(T));
        shared_data->producer_futex.fetch_add(1, std::memory_order_release);
        futexWakeAll(&shared_data->producer_futex);
    }


private:

    bool openMap()
    {
        log("Opening memory mapped file");    
        // open the mapped memory file descriptor
        int fd;

        // prepare the file descriptor with /tmp/
        // Check if /tmp/ is already in the file name
        if (mapped_file.find("/tmp/") == std::string::npos)
        {
            mapped_file = "/tmp/" + mapped_file;
        }

        // if (isProducer)
        // {
            fd = open(mapped_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        // } else {
        //     fd = open(mapped_file.c_str(), O_RDWR);
        // }

        if (fd == -1) {
            log("Error opening/creating memory-mapped file");
            return 1;
        }

        // if (isProducer)
        // {
            // Check if file needs to be truncated
            struct stat fileStat;
            if (stat(mapped_file.c_str(), &fileStat) == 0)
            {
                off_t currentSize = fileStat.st_size;
                if (currentSize != sizeof(shared_data_t))
                {
                    if (ftruncate(fd, sizeof(shared_data_t)) == -1)
                    {
                        logError("Error truncating Memmory Map");
                        close(fd);
                        return true;
                    } 
                    else
                    {
                        log("Memmory Map truncated successfully");
                    }
                }
                else
                {
                    log("Memmory Map already at the correct size");
                }
            }
            else
            {
                log("Error getting Memmory Map stat");
                close(fd);
                return true;
            }
        // }

        // Map the memory
        shared_data = static_cast<shared_data_t*>(mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0u));
        if (shared_data == MAP_FAILED) {
            log("Error mapping memory");
            close(fd);
            return true;
        } else {
            log("Memory mapped successfully");
        }

        if (isProducer)
        {
            shared_data->producer_futex.store(1, std::memory_order_release);
            shared_data->triple_buffer[0] = T();
            shared_data->triple_buffer[1] = T();
            shared_data->triple_buffer[2] = T();
        }
        return 0;
    }

    void closeMap()
    {
        // close memory mapped file
        if (munmap(shared_data, sizeof(shared_data_t)) == -1) {
            log("Error un-mmapping the file");
        }
    }

    void log(std::string event)
    {
        //std::cout << "[" << mapped_file << "] " << event << "\n";
    }

    void logError(std::string event)
    {
        std::cout << "[" << mapped_file << "] " << event << "\n";
    }

    bool futexWait(std::atomic<int>* addr, int val)
    {
        int ret;
        do {
            ret = syscall(SYS_futex, addr, FUTEX_WAIT, val, &timeout, NULL, 0);
            if (ret == -1) {
                if (errno == EINTR) {
                    // Interrupted by a signal, continue waiting
                    continue;
                } else if (errno == ETIMEDOUT) {
                    // log("Timeout in futexWait");
                    return false;
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // The operation was interrupted, the timeout expired, or the futex was not the expected value
                    log("EAGAIN or EWOULDBLOCK in futexWait");
                    return true;
                } else {
                    // Other futex error occurred
                    log("Error in futexWait: " + std::to_string(errno));
                    return false;
                }
            }
        } while (ret != 0);

        return true;
    }

    void futexWakeAll(std::atomic<int>* addr)
    {
        int ret;
        do {
            ret = syscall(SYS_futex, addr, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
        } while (ret == -1 && errno == EINTR);
    }

    // struct of shared data
    struct shared_data_t {
        std::atomic<int> producer_futex;
        T triple_buffer[3];
    };

    struct timespec timeout;

    // shared memory
    std::string mapped_file;
    int fd;
    std::atomic<bool> memory_mapped;

    // Shared data structure type from template
    shared_data_t* shared_data;

    // isProducer flag
    bool isProducer;
};
}
#endif /* SDATA_HPP_INCLUDED */