#pragma once

#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>

namespace app
{

class spin_lock
{
public:

    spin_lock()
        : flg_(ATOMIC_FLAG_INIT)
    {
    }
    ~spin_lock() = default;

public:

    void lock()
    {
        for (; flg_.test_and_set(std::memory_order_acquire););
    }

    bool try_lock()
    {
        int _try_cnt = 100;
        for (; _try_cnt > 0 && flg_.test_and_set(std::memory_order_acquire); --_try_cnt);
        return _try_cnt > 0;
    }

    void unlock()
    {
        flg_.clear(std::memory_order_release);
    }

private:

    spin_lock(const spin_lock&) = delete;
    spin_lock(spin_lock&&) = delete;
    spin_lock& operator=(const spin_lock&) = delete;
    spin_lock& operator=(spin_lock&&) = delete;

private:

    std::atomic_flag        flg_;
};


class spin_mutex
{
public:

    spin_mutex() : flg_(ATOMIC_FLAG_INIT)
    {
    }

    explicit spin_mutex(int _try_cnt)
        : flg_(ATOMIC_FLAG_INIT), try_cnt_(_try_cnt)
    {
    }

    ~spin_mutex() = default;

public:

    void lock()
    {
        for (int millis = 5;; millis = (millis < 100 ? millis << 2 : millis))
        {
            int _try_cnt = try_cnt_;
            for (; _try_cnt > 0 && flg_.test_and_set(std::memory_order_acquire); --_try_cnt);
            if (_try_cnt > 0) {
                return;
            }

            std::unique_lock<std::mutex> ulk(mtx_);
            cond_.wait_for(ulk, std::chrono::milliseconds(millis));
        }
    }

    bool try_lock()
    {
        int _try_cnt = try_cnt_;
        for (; _try_cnt > 0 && flg_.test_and_set(std::memory_order_acquire); --_try_cnt);
        return _try_cnt > 0;
    }

    void unlock()
    {
        flg_.clear(std::memory_order_release);
        cond_.notify_all();
    }

private:

    spin_mutex(const spin_mutex&) = delete;
    spin_mutex(spin_mutex&&) = delete;
    spin_mutex& operator=(const spin_mutex&) = delete;
    spin_mutex& operator=(spin_mutex&&) = delete;

private:

    std::mutex              mtx_;
    std::atomic_flag        flg_;
    std::condition_variable cond_;
    int try_cnt_            { 200 };
};

};