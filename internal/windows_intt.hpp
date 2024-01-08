#ifndef __SCC_WINDOWS_INTT_HPP__
#define __SCC_WINDOWS_INTT_HPP__
#pragma once

#include <Windows.h>
#include <atomic>

class InterruptibleThread
{
public:
    InterruptibleThread() : m_thread(nullptr), m_interrupted(false), m_statusCode(0) {}
    InterruptibleThread(const InterruptibleThread &) = delete;
    InterruptibleThread(InterruptibleThread &&other) noexcept
        : m_thread{[&other]() noexcept -> HANDLE
                   {
                       std::lock_guard<std::mutex> lock(other.m_mutex);
                       const auto &ret = other.m_thread;
                       other.m_thread = nullptr;
                       other.m_interrupted.store(false, std::memory_order_release);
                       other.m_statusCode.store(0, std::memory_order_release);
                       return ret;
                   }()},
          m_interrupted(other.m_interrupted.load(std::memory_order_acquire)),
          m_statusCode(other.m_statusCode.load(std::memory_order_acquire)) {}
    InterruptibleThread &operator=(const InterruptibleThread &) = delete;
    InterruptibleThread &operator=(InterruptibleThread &&other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            std::lock_guard<std::mutex> lock(other.m_mutex);
            m_thread = other.m_thread;
            other.m_thread = nullptr;
            m_interrupted.store(other.m_interrupted.load(std::memory_order_acquire), std::memory_order_release);
            m_statusCode.store(other.m_statusCode.load(std::memory_order_acquire), std::memory_order_release);
        }
        return *this;
    }

    virtual ~InterruptibleThread()
    {
        if (m_thread)
            CloseHandle(m_thread);
    }

    void start()
    {
        m_thread = CreateThread(NULL, 0, &InterruptibleThread::threadFunc, this, 0, NULL);
    }

    void suspend()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_thread != nullptr)
        {
            SuspendThread(m_thread);
        }
    }

    void resume()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_thread != nullptr)
        {
            ResumeThread(m_thread);
        }
    }

    void interrupt()
    {
        m_interrupted.store(true, std::memory_order_release);
        suspend(); // Suspend the thread for interruption
    }

    void join()
    {
        WaitForSingleObject(m_thread, INFINITE);
    }

    DWORD getId() const
    {
        return GetThreadId(m_thread);
    }

    void setStatusCode(int code)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_statusCode = code;
    }

    int getStatusCode() const
    {
        return m_statusCode;
    }

protected:
    virtual void run() = 0;
    virtual void onInterrupt() = 0;

private:
    static DWORD WINAPI threadFunc(LPVOID arg)
    {
        InterruptibleThread *thread = static_cast<InterruptibleThread *>(arg);
        thread->run();

        if (thread->m_interrupted.load())
        {
            thread->onInterrupt();
        }

        return 0;
    }

protected:
    HANDLE m_thread;
    std::atomic<bool> m_interrupted;
    std::atomic<int> m_statusCode;
    mutable std::mutex m_mutex;
};

class InterruptibleThreadManager
{
public:
    static InterruptibleThreadManager &getInstance()
    {
        static InterruptibleThreadManager instance;
        return instance;
    }

    InterruptibleThreadManager(const InterruptibleThreadManager &) = delete;
    InterruptibleThreadManager(InterruptibleThreadManager &&) = delete;
    InterruptibleThreadManager &operator=(const InterruptibleThreadManager &) = delete;
    InterruptibleThreadManager &operator=(InterruptibleThreadManager &&) = delete;

    void registerThread(unsigned long id, InterruptibleThread *thread)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_[id] = thread;
    }

    void registerThread(unsigned long id, std::unique_ptr<InterruptibleThread> &&thread)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_[id] = thread.release();
    }

    void unregisterThread(unsigned long id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_.erase(id);
    }

    void startThread(unsigned long id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_[id]->start();
    }

    void suspendThread(unsigned long id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_[id]->suspend();
    }

    void resumeThread(unsigned long id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_[id]->resume();
    }

    void interruptThread(unsigned long id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_[id]->interrupt();
    }

    void joinThread(unsigned long id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        threads_[id]->join();
    }

private:
    InterruptibleThreadManager() = default;
    ~InterruptibleThreadManager() = default;

private:
    std::mutex mutex_;
    std::unordered_map<unsigned long, InterruptibleThread *> threads_;
};

#endif // __SCC_WINDOWS_INTT_HPP__
