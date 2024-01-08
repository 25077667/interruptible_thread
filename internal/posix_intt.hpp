#ifndef __SCC_POSIX_INTT_HPP__
#define __SCC_POSIX_INTT_HPP__
#pragma once

#include <pthread.h>
#include <signal.h>

#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>

class InterruptibleThread
{
public:
    InterruptibleThread() : m_thread(0), m_interrupted(false) {}
    InterruptibleThread(const InterruptibleThread &) = delete;
    InterruptibleThread(InterruptibleThread &&other) noexcept
        : m_thread{[&other]() noexcept -> pthread_t
                   {
                       std::lock_guard<std::mutex> lock(other.m_mutex);
                       const auto &ret = other.m_thread;
                       other.m_thread = 0;
                       return ret;
                   }()},
          m_interrupted(other.m_interrupted.load(std::memory_order_acquire)) {}
    InterruptibleThread &operator=(const InterruptibleThread &) = delete;
    InterruptibleThread &operator=(InterruptibleThread &&other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            std::lock_guard<std::mutex> lock(other.m_mutex);
            m_thread = other.m_thread;
            other.m_thread = 0;
            m_interrupted.store(other.m_interrupted.load(std::memory_order_acquire), std::memory_order_release);
        }
        return *this;
    }
    virtual ~InterruptibleThread()
    {
        cleanup();
    }

    void start()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_thread != 0)
            return;
        if (pthread_create(&m_thread, nullptr, &InterruptibleThread::threadFunc, this) != 0)
        {
            // Handle error (throw exception or other)
        }
    }

    void interrupt()
    {
        m_interrupted.store(true, std::memory_order_release);
        pthread_kill(m_thread, SIGUSR1); // Ensure proper signal handling
    }

    void join()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_thread != 0)
        {
            pthread_join(m_thread, nullptr);
            m_thread = 0;
        }
    }

    void suspend()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_thread != 0)
        {
            pthread_kill(m_thread, SIGSTOP);
        }
    }

    void resume()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_thread != 0)
        {
            pthread_kill(m_thread, SIGCONT);
        }
    }

    int getId() const
    {
        return m_thread;
    }

protected:
    virtual void run() = 0;
    virtual void onInterrupt() = 0;

private:
    static void *threadFunc(void *arg)
    {
        InterruptibleThread *thread = static_cast<InterruptibleThread *>(arg);
        thread->run();

        if (thread->m_interrupted.load())
        {
            thread->onInterrupt();
        }

        return nullptr;
    }

    void cleanup()
    {
        if (m_thread != 0)
        {
            pthread_cancel(m_thread);
            pthread_join(m_thread, nullptr);
            m_thread = 0;
        }
    }

protected:
    pthread_t m_thread;
    std::atomic<bool> m_interrupted;
    std::mutex m_mutex; // Added mutex for thread-safety
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

    void interruptThread(unsigned long id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = threads_.find(id);
        if (it != threads_.end())
        {
            it->second->interrupt();
        }
    }

    // startThread() and joinThread() are not thread-safe
    void startThread(unsigned long id)
    {
        auto it = threads_.find(id);
        if (it != threads_.end())
        {
            it->second->start();
        }
    }

    // startThread() and joinThread() are not thread-safe
    void joinThread(unsigned long id)
    {
        auto it = threads_.find(id);
        if (it != threads_.end())
        {
            it->second->join();
        }
    }

    // suspendThread() and resumeThread() are not thread-safe
    void suspendThread(unsigned long id)
    {
        auto it = threads_.find(id);
        if (it != threads_.end())
        {
            it->second->suspend();
        }
    }

    // suspendThread() and resumeThread() are not thread-safe
    void resumeThread(unsigned long id)
    {
        auto it = threads_.find(id);
        if (it != threads_.end())
        {
            it->second->resume();
        }
    }

private:
    InterruptibleThreadManager() {}

    std::unordered_map<unsigned long, InterruptibleThread *> threads_;
    std::mutex mutex_;
};

#endif // __SCC_POSIX_INTT_HPP__
