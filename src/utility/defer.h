#ifndef DEFER_H
#define DEFER_H

template <typename F> 
struct Defer {
    F f; 
    ~Defer() noexcept { f(); }
};

#define DEFER_CONCAT_IMPL(a, b) a##b
#define DEFER_CONCAT(a, b) DEFER_CONCAT_IMPL(a, b)
#define defer(fn) Defer DEFER_CONCAT(_defer_, __LINE__){[&]{ fn; }}

#endif