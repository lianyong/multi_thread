#pragma once

#include <unordered_map>
#include <mutex>

namespace app
{

template<typename _Key,
         typename _Tp,
         typename _Hash = hash<_Key>,
         typename _Pred = std::equal_to<_Key>,
         typename _Alloc = std::allocator<std::pair<const _Key, _Tp> > >

class unordered_map
{
private:
    std::unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>      map_;
    mutable std::recursive_mutex                             mtx_;

public:
    using map_type              = std::unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>;
    using key_type              = typename map_type::key_type;
    using mapped_type           = typename map_type::mapped_type;
    using value_type            = typename map_type::value_type;
    using hasher                = typename map_type::hasher;
    using key_equal             = typename map_type::key_equal;
    using allocator_type        = typename map_type::allocator_type;
    using reference             = typename map_type::reference;
    using const_reference       = typename map_type::const_reference;
    using pointer               = typename map_type::pointer;
    using const_pointer         = typename map_type::const_pointer;
    using iterator              = typename map_type::iterator;
    using const_iterator        = typename map_type::const_iterator;
    using local_iterator        = typename map_type::local_iterator;
    using const_local_iterator  = typename map_type::const_local_iterator;
    using size_type             = typename map_type::size_type;
    using difference_type       = typename map_type::difference_type;

    typedef std::unique_lock<std::recursive_mutex>  lock;

    unordered_map() = default;
    unordered_map(const unordered_map&) = delete;
    unordered_map(unordered_map&&) = default;
    unordered_map& operator=(const unordered_map&) = delete;
    unordered_map& operator=(unordered_map&&) = delete;

    explicit unordered_map(size_type __n,
                           const hasher& __hf = hasher(),
                           const key_equal& __eql = key_equal(),
                           const allocator_type& __a = allocator_type())
        : map_(__n, __hf, __eql, __a)
    {}

    template<typename _InputIterator>
    unordered_map(_InputIterator __first, _InputIterator __last,
                  size_type __n = 0,
                  const hasher& __hf = hasher(),
                  const key_equal& __eql = key_equal(),
                  const allocator_type& __a = allocator_type())
        : map_(__first, __last, __n, __hf, __eql, __a)
    {}

    unordered_map(const map_type& v)
        : map_(v)
    {}

    unordered_map(map_type&& rv)
        : map_(std::move(rv))
    {}

    explicit unordered_map(const allocator_type& __a)
        : map_(__a)
    {}

    unordered_map(const map_type& __umap,
                  const allocator_type& __a)
        : map_(__umap, __a)
    {}

    unordered_map(map_type&& __umap,
                  const allocator_type& __a)
        : map_(std::move(__umap), __a)
    {}

    unordered_map(initializer_list<value_type> __l,
                  size_type __n = 0,
                  const hasher& __hf = hasher(),
                  const key_equal& __eql = key_equal(),
                  const allocator_type& __a = allocator_type())
        : map_(__l, __n, __hf, __eql, __a)
    {}

    unordered_map(size_type __n, const allocator_type& __a)
        : unordered_map(__n, hasher(), key_equal(), __a)
    {}

    unordered_map(size_type __n, const hasher& __hf,
                  const allocator_type& __a)
        : unordered_map(__n, __hf, key_equal(), __a)
    {}

    template<typename _InputIterator>
    unordered_map(_InputIterator __first, _InputIterator __last,
                  size_type __n,
                  const allocator_type& __a)
        : map_(__first, __last, __n, __a)
    {}

    template<typename _InputIterator>
    unordered_map(_InputIterator __first, _InputIterator __last,
                  size_type __n, const hasher& __hf,
                  const allocator_type& __a)
        : unordered_map(__first, __last, __n, __hf, key_equal(), __a)
    {}

    unordered_map(initializer_list<value_type> __l,
                  size_type __n,
                  const allocator_type& __a)
        : unordered_map(__l, __n, hasher(), key_equal(), __a)
    {}

    unordered_map(initializer_list<value_type> __l,
                  size_type __n, const hasher& __hf,
                  const allocator_type& __a)
        : unordered_map(__l, __n, __hf, key_equal(), __a)
    {}

    bool empty() const noexcept {
        lock lck(mtx_);
        return map_.empty();
    }

    size_type size() const noexcept {
        lock lck(mtx_);
        return map_.size();
    }

    size_type max_size() const noexcept {
        lock lck(mtx_);
        return map_.max_size();
    }

    iterator begin() noexcept {
        lock lck(mtx_);
        return map_.begin();
    }

    const_iterator begin() const noexcept {
        lock lck(mtx_);
        return map_.begin();
    }

    const_iterator cbegin() const noexcept {
        lock lck(mtx_);
        return map_.cbegin();
    }

    iterator end() noexcept {
        lock lck(mtx_);
        return map_.end();
    }

    const_iterator end() const noexcept {
        lock lck(mtx_);
        return map_.end();
    }

    const_iterator cend() const noexcept {
        lock lck(mtx_);
        return map_.cend();
    }

    template<typename... _Args>
    std::pair<iterator, bool>
    emplace(_Args&& ... __args) {
        lock lck(mtx_);
        return map_.emplace(std::forward<_Args>(__args)...);
    }

    template<typename... _Args>
    iterator
    emplace_hint(const_iterator __pos, _Args&& ... __args) {
        lock lck(mtx_);
        return map_.emplace_hint(__pos, std::forward<_Args>(__args)...);
    }

    std::pair<iterator, bool> insert(const value_type& __x) {
        lock lck(mtx_);
        return map_.insert(__x);
    }

    template < typename _Pair, typename = typename
               std::enable_if < std::is_constructible < value_type,
                                _Pair && >::value >::type >
    std::pair<iterator, bool>
    insert(_Pair && __x) {
        lock lck(mtx_);
        return map_.insert(std::forward<_Pair>(__x));
    }

    iterator
    insert(const_iterator __hint, const value_type& __x) {
        lock lck(mtx_);
        return map_.insert(__hint, __x);
    }

    template < typename _Pair, typename = typename
               std::enable_if < std::is_constructible < value_type,
                                _Pair && >::value >::type >
    iterator
    insert(const_iterator __hint, _Pair && __x) {
        lock lck(mtx_);
        return map_.insert(__hint, std::forward<_Pair>(__x));
    }

    template<typename _InputIterator>
    void
    insert(_InputIterator __first, _InputIterator __last) {
        lock lck(mtx_);
        map_.insert(__first, __last);
    }

    void insert(initializer_list<value_type> __l) {
        lock lck(mtx_);
        map_.insert(__l);
    }

    iterator erase(const_iterator __position) {
        lock lck(mtx_);
        return map_.erase(__position);
    }

    iterator erase(iterator __position) {
        lock lck(mtx_);
        return map_.erase(__position);
    }

    size_type erase(const key_type& __x) {
        lock lck(mtx_);
        return map_.erase(__x);
    }

    iterator erase(const_iterator __first, const_iterator __last) {
        lock lck(mtx_);
        return map_.erase(__first, __last);
    }

    void clear() noexcept {
        lock lck(mtx_);
        map_.clear();
    }

    void swap(map_type& __x) noexcept(noexcept(map_.swap(__x._M_h))) {
        lock lck(mtx_);
        map_.swap(__x._M_h);
    }

    hasher hash_function() const {
        lock lck(mtx_);
        return map_.hash_function();
    }

    key_equal key_eq() const {
        lock lck(mtx_);
        return map_.key_eq();
    }

    iterator find(const key_type& __x) {
        lock lck(mtx_);
        return map_.find(__x);
    }

    const_iterator find(const key_type& __x) const {
        lock lck(mtx_);
        return map_.find(__x);
    }

    size_type count(const key_type& __x) const {
        lock lck(mtx_);
        return map_.count(__x);
    }

    std::pair<iterator, iterator> equal_range(const key_type& __x) {
        lock lck(mtx_);
        return map_.equal_range(__x);
    }

    std::pair<const_iterator, const_iterator>
    equal_range(const key_type& __x) const {
        lock lck(mtx_);
        return map_.equal_range(__x);
    }

    mapped_type& operator[](const key_type& __k) {
        lock lck(mtx_);
        return map_[__k];
    }

    mapped_type& operator[](key_type&& __k) {
        lock lck(mtx_);
        return map_[std::move(__k)];
    }

    mapped_type& at(const key_type& __k) {
        lock lck(mtx_);
        return map_.at(__k);
    }

    const mapped_type& at(const key_type& __k) const {
        lock lck(mtx_);
        return map_.at(__k);
    }

    size_type bucket_count() const noexcept {
        lock lck(mtx_);
        return map_.bucket_count();
    }

    size_type max_bucket_count() const noexcept {
        lock lck(mtx_);
        return map_.max_bucket_count();
    }

    size_type bucket_size(size_type __n) const {
        lock lck(mtx_);
        return map_.bucket_size(__n);
    }

    size_type bucket(const key_type& __key) const {
        lock lck(mtx_);
        return map_.bucket(__key);
    }

    local_iterator begin(size_type __n) {
        lock lck(mtx_);
        return map_.begin(__n);
    }

    const_local_iterator begin(size_type __n) const {
        lock lck(mtx_);
        return map_.begin(__n);
    }

    const_local_iterator cbegin(size_type __n) const {
        lock lck(mtx_);
        return map_.cbegin(__n);
    }

    local_iterator end(size_type __n) {
        lock lck(mtx_);
        return map_.end(__n);
    }

    const_local_iterator end(size_type __n) const {
        lock lck(mtx_);
        return map_.end(__n);
    }

    const_local_iterator cend(size_type __n) const {
        lock lck(mtx_);
        return map_.cend(__n);
    }

    float load_factor() const noexcept {
        lock lck(mtx_);
        return map_.load_factor();
    }

    float max_load_factor() const noexcept {
        lock lck(mtx_);
        return map_.max_load_factor();
    }

    void max_load_factor(float __z) {
        lock lck(mtx_);
        map_.max_load_factor(__z);
    }

    void rehash(size_type __n) {
        lock lck(mtx_);
        map_.rehash(__n);
    }

    void reserve(size_type __n) {
        lock lck(mtx_);
        map_.reserve(__n);
    }

    bool find(const key_type& __x, mapped_type& value) const {
        lock lck(mtx_);
        auto it = map_.find(__x);
        auto found = it != map_.end();
        if (found) {
            value = it->second;
        }
        return found;
    }

    lock get_lock()const noexcept {
        return std::move(lock(mtx_));
    }

    /* if not find then insert otherwise do nothing */
    mapped_type try_insert(const key_type& key, const mapped_type& value) {
        lock lck(mtx_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            map_.insert(std::move(value_type(key, value)));
            return value;
        }
        return it->second;
    }

    bool try_insert(const key_type& key, mapped_type&& value) {
        lock lck(mtx_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            map_.insert(std::move(value_type(key, std::move(value))));
            return true;
        }
        return false;
    }

    std::shared_ptr<mapped_type> replace(const key_type& key, const mapped_type& value) {
        lock lck(mtx_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            auto ret = std::make_shared<mapped_type>(it->second);
            map_.insert(std::move(value_type(key, value)));
            return ret;
        }
        return std::shared_ptr<mapped_type>();
    }

    bool replace(const key_type& key, const mapped_type& value, const mapped_type& newvalue) {
        lock lck(mtx_);
        auto it = map_.find(key);
        if (it != map_.end() && it->second == value) {
            map_.insert(std::move(value_type(key, newvalue)));
            return true;
        }
        return false;
    }

    template<typename _Key1, typename _Tp1, typename _Hash1, typename _Pred1,
             typename _Alloc1>
    friend bool
    operator==(const unordered_map<_Key1, _Tp1, _Hash1, _Pred1, _Alloc1>&,
               const unordered_map<_Key1, _Tp1, _Hash1, _Pred1, _Alloc1>&);
};

template<class _Key, class _Tp, class _Hash, class _Pred, class _Alloc>
inline bool
operator==(const unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& __x,
           const unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& __y)
{
    auto guardx = __x.get_lock();
    auto guardy = __y.get_lock();
    return __x.map_ == __y.map_;
}

template<class _Key, class _Tp, class _Hash, class _Pred, class _Alloc>
inline bool
operator!=(const unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& __x,
           const unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& __y)
{
    return !(__x == __y);
}

};
