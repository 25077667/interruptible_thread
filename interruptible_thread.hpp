#ifndef __SCC_INTERRUPTABLE_THREAD_HPP__
#define __SCC_INTERRUPTABLE_THREAD_HPP__
#pragma once

#ifdef _WIN32
#include <internal/windows_intt.hpp>
#elif __linux__ || __APPLE__ || __FreeBSD__
#include <internal/posix_intt.hpp>
#else
#error "Unsupported platform"
#endif

#ifdef DEBUG
#include <iostream>
#include <thread>
#else
#include <iostream>
#include <thread>
#endif

class RegularInterruptibleThread : public InterruptibleThread
{
public:
    RegularInterruptibleThread() = default;
    virtual ~RegularInterruptibleThread() override {}
    RegularInterruptibleThread(const RegularInterruptibleThread &) = delete;
    RegularInterruptibleThread &operator=(const RegularInterruptibleThread &) = delete;
    RegularInterruptibleThread(RegularInterruptibleThread &&) = default;
    RegularInterruptibleThread &operator=(RegularInterruptibleThread &&) = default;

protected:
    virtual void run() override
    {
        while (!m_interrupted.load())
        {
            // Regular task execution
            std::cout << "Running regular task..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate work
        }
    }

    virtual void onInterrupt() override
    {
        std::cout << "Thread interrupted. Cleaning up resources..." << std::endl;
        // Perform necessary cleanup and resource release
    }
};

#endif // __SCC_INTERRUPTABLE_THREAD_HPP__