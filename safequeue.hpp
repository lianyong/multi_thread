#pragma once

#include <mutex>
#include <memory>
#include <condition_variable>
#include <deque>
#include <queue>

namespace app
{

template<typename T, typename _Container>
class safe_queue_base
{
protected:
    typedef _Container                      Container;
    typedef std::lock_guard<std::mutex>     lck_grd;
    typedef std::unique_lock<std::mutex>    unq_lck;

public:
    safe_queue_base() = default;
    ~safe_queue_base() = default;

    void push(T t)
    {
        lck_grd lck(mtx_);
        queue_.push(std::move(t));
        ++size_;
        cond_.notify_one();
    }

    void wait_and_pop(T& val)
    {
        unq_lck lck(mtx_);
        cond_.wait(lck, [this]() {
            return !queue_.empty();
        });

        val = std::move(queue_.front());
        queue_.pop();
        --size_;
    }

    bool wait_and_pop(T& val, const std::chrono::milliseconds& timer)
    {
        unq_lck lck(mtx_);
        cond_.wait_for(lck, timer, [this]() {
            return !queue_.empty();
        });

        if (queue_.empty())
        {
            return false;
        }

        val = std::move(queue_.front());
        queue_.pop();
        --size_;
        return true;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        unq_lck lck(mtx_);
        cond_.wait(lck, [this]() {
            return !queue_.empty();
        });

        auto val = std::make_shared<T>(std::move(queue_.front()));
        queue_.pop();
        --size_;
        return val;
    }

    std::shared_ptr<T> wait_and_pop(const std::chrono::milliseconds& timer)
    {
        unq_lck lck(mtx_);
        cond_.wait_for(lck, timer, [this]() {
            return !queue_.empty();
        });

        auto val = std::make_shared<T>(std::move(queue_.front()));
        queue_.pop();
        --size_;
        return val;
    }

    bool try_pop(T& val)
    {
        lck_grd lck(mtx_);
        if (queue_.empty()) {
            return false;
        }

        val = std::move(queue_.front());
        queue_.pop();
        --size_;
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        lck_grd lck(mtx_);
        if (queue_.empty()) {
            return std::shared_ptr<T>();
        }

        auto val = std::make_shared<T>(std::move(queue_.front()));
        queue_.pop();
        --size_;
        return val;
    }

    inline bool empty() const
    {
        return size_ != 0;
    }

    inline size_t size() const
    {
        return size_;
    }

protected:
    Container               queue_;
    std::mutex              mtx_;
    std::condition_variable cond_;
    std::atomic<size_t>     size_;
};

template<typename T>
class priority_queue
    : public std::priority_queue<T, std::vector<T>, std::less<T> >
{
public:
    typedef const T&    const_reference;
    typedef T&          reference;

    const_reference front() const
    {
        return top();
    }
};

template<class T>
using safe_queue = safe_queue_base<T, std::deque<T>>;

template<class T>
using safe_priorqueue = safe_queue_base<T, priority_queue<T>>;

};
