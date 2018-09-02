#pragma once

#if 1
#   define utils_assert_ex(exp, fmt, ...)   do { if(exp) break; fprintf(stderr, "dbg::[%s][%s:%d][%s]" fmt "\n", __FILE__, __FUNCTION__, __LINE__, #exp, ##__VA_ARGS__);  exit(1); } while(0)
#   define utils_assert(exp)                utils_assert_ex(exp, "fatal error")
#   define utils_syntax(exp)                exp
#else
#   include <cassert>
#   define utils_assert_ex(exp, ...)        assert(exp)
#   define utils_assert(exp)                utils_assert_ex(exp)
#   define utils_syntax(exp)                (void)0
#endif

#if (defined(_WIN32) || __cplusplus >= 201103L)
#include <mutex>
#include <condition_variable>

namespace app
{
// support c++11 but not support c++17 then implement shared_mutex
class shared_mutex
{
protected:
    typedef std::mutex                          mutex;
    typedef std::unique_lock<mutex>             unique_lock;
    typedef std::lock_guard<mutex>              lock_guard;
    typedef std::thread::id						trd_id;
    typedef std::map<trd_id, size_t>            map_depth;

public:
    shared_mutex() = default;
    ~shared_mutex() = default;

public:
    void lock_shared()
    {
        unique_lock lck(mtx_);
        cond_r_.wait(lck, [&]() {
            return readable();
        });
        read();
    }

    bool try_lock_shared()
    {
        lock_guard lck(mtx_);
        if (readable()) {
            read();
            return true;
        }

        return false;
    }

    void unlock_shared()
    {
        trd_id id = std::this_thread::get_id();
        lock_guard lck(mtx_);
        utils_assert(read_cnt_> 0);

        --read_cnt_;
        auto it = read_depth_.find(id);
        utils_assert(it == read_depth_.end() || it->second > 0);

        size_t r_depth = --it->second;
        if (r_depth == 0)
        {
            read_depth_.erase(it);
        }

        if (write_depth_ > 0) {
            return;
        }

        if (r_depth == 0) {
            w_id_ = trd_id();
            if (write_cnt_ > 0) {
                return cond_w_.notify_one();
            }
        }

        if(write_cnt_ == 0) {
            return cond_r_.notify_all();
        }
    }

    void lock()
    {
        trd_id id = std::this_thread::get_id();
        unique_lock lck(mtx_);
        ++write_cnt_;

        cond_w_.wait(lck, [&]() {
            return writeable();
        });

        write();
    }

    bool try_lock()
    {
        trd_id id = std::this_thread::get_id();
        lock_guard lck(mtx_);
        if (writeable()) {
            ++write_cnt_;
            write();
            return true;
        }

        return false;
    }

    void unlock()
    {
        trd_id id = std::this_thread::get_id();
        lock_guard lck(mtx_);
        utils_assert(write_cnt_ > 0);

        --write_cnt_;
        if (w_id_ == id) {
            if (--write_depth_ != 0) {
                return;
            }
        }

        if (read_depth_.find(id) == read_depth_.end()) {
            w_id_ = trd_id();

            if (write_cnt_ > 0) {
                return cond_w_.notify_one();
            }
        }

        if (write_cnt_ == 0) {
            cond_r_.notify_all();
        }
    }

    std::mutex::native_handle_type native_handle()
    {
        return mtx_.native_handle();
    }

private:

    inline bool readable()
    {
        /* 获得读的权限
        1. 没有写的线程;
        2. 当前线程正在写，同时读写；
        */

        if (write_cnt_ == 0)
        {
            return true;
        }

        if (w_id_ == std::this_thread::get_id())
        {
            return true;
        }

        return false;
    }

    inline bool writeable()
    {
        /* 获得写的权限
        1. 没有其他读进程，只有当前进程写；
        2. 当前线程正在写，递归写；
        3. 当前线程正在读，不存在其他读线程；
        */

        if (read_cnt_ == 0 && write_cnt_ == 1)
        {
            return true;
        }

        trd_id id = std::this_thread::get_id();
        if (w_id_ == id)
        {
            return true;
        }

        if (read_depth_.size() == 1 && read_depth_.cbegin()->first == id)
        {
            return true;
        }

        return false;
    }

    inline void read()
    {
        ++read_cnt_;
        ++read_depth_[std::this_thread::get_id()];
    }

    inline void write()
    {
        ++write_depth_;
        w_id_ = std::this_thread::get_id();
    }

private:
    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

protected:
    volatile size_t read_cnt_		{ 0 };		// 读者数量
    volatile size_t write_cnt_		{ 0 };		// 写者数量
    volatile size_t write_depth_	{ 0 };		// 写者深度
    mutable trd_id					w_id_;		// 当前写者
    mutable mutex					mtx_;		// 互斥锁
    mutable map_depth				read_depth_;// 读者深度
    std::condition_variable			cond_w_;	// 写者条件
    std::condition_variable			cond_r_;	// 读者条件
};

}
#else
// todo: not support c++11 then implement shared_mutex base on pthread
namespace app
{

};
#endif

namespace app
{

template <typename shared_lock>
class scoped_write_guard
{
public:
    explicit scoped_write_guard(shared_lock& rw_)
        : rw_lockable_(rw_)
    {
        rw_lockable_.lock();
    }

    ~scoped_write_guard()
    {
        rw_lockable_.unlock();
    }

private:
    scoped_write_guard() = delete;
    scoped_write_guard(const scoped_write_guard&) = delete;
    scoped_write_guard& operator=(const scoped_write_guard&) = delete;

private:
    shared_lock& rw_lockable_;
};


template <typename shared_lock>
class scoped_read_guard
{
public:
    explicit scoped_read_guard(shared_lock& rw_)
        : rw_lockable_(rw_)
    {
        rw_lockable_.lock_shared();
    }

    ~scoped_read_guard()
    {
        rw_lockable_.unlock_shared();
    }

private:
    scoped_read_guard() = delete;
    scoped_read_guard(const scoped_read_guard&) = delete;
    scoped_read_guard& operator=(const scoped_read_guard&) = delete;

private:
    shared_lock& rw_lockable_;
};

template <typename shared_lock>
class unique_write_guard
{
public:
    explicit unique_write_guard(shared_lock& rw_)
        : shared_lock_(rw_), owns_(false)
    {
        lock();
    }

    ~unique_write_guard()
    {
        if (owns_)
        {
            shared_lock_.unlock();
        }
    }

    void lock()
    {
        utils_assert(!owns_);
        shared_lock_.lock();
        owns_ = true;
    }

    void unlock()
    {
        utils_assert(owns_);
        shared_lock_.unlock();
        owns_ = false;
    }

    bool try_lock()
    {
        utils_assert(!owns_);
        return owns_ = shared_lock_.try_lock();;
    }


    unique_write_guard(const unique_write_guard& other)
        : shared_lock_(other.rw_), owns_(false)
    {
        if (other.owns_)
        {
            other.unlock();
            lock();
        }
    }

    unique_write_guard(unique_write_guard&& other)
        : shared_lock_(other.shared_lock_), owns_(other.owns_)
    {
        other.owns_ = false;
    }

private:
    unique_write_guard() = delete;
    unique_write_guard& operator=(const unique_write_guard&) = delete;

private:
    shared_lock&    shared_lock_;
    bool            owns_;
};

/* recursive_mutex for read mutex */
template <typename shared_lock>
class unique_read_guard
{
public:
    explicit unique_read_guard(shared_lock& rw_)
        : shared_lock_(rw_), owns_ref_(0)
    {
        lock();
    }

    ~unique_read_guard()
    {
        while (owns_ref_-- > 0)
        {
            shared_lock_.unlock_shared();
        }
    }

    void lock()
    {
        shared_lock_.lock_shared();
        ++owns_ref_;
    }

    void unlock()
    {
        utils_assert(owns_ref_ > 0);
        shared_lock_.unlock_shared();
        --owns_ref_;
    }

    bool try_lock()
    {
        if (shared_lock_.try_lock_shared())
        {
            ++owns_ref_;
            return true;
        }

        return false;
    }

    unique_read_guard(const unique_read_guard& other)
        : shared_lock_(other.rw_), owns_ref_(0)
    {
        lock();
    }

    unique_read_guard(unique_read_guard&& other)
        : shared_lock_(other.rw_), owns_ref_(other.owns_ref_)
    {
        other.owns_ref_ = 0;
    }

private:
    unique_read_guard() = delete;
    unique_read_guard& operator=(const unique_read_guard&) = delete;

private:
    shared_lock&    shared_lock_;
    int             owns_ref_;
};
};

/* not thread write safe */
typedef app::scoped_write_guard<app::shared_mutex>  scoped_wguard;
typedef app::scoped_read_guard<app::shared_mutex>   scoped_rguard;

typedef app::unique_write_guard<app::shared_mutex>  unique_wguard;
typedef app::unique_read_guard<app::shared_mutex>   unique_rguard;
