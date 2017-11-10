#include <optional>

#define TRY(expr...) ({ \
    auto && __try_var__ = (expr); \
    \
    if(!__try_var__) {\
        return extract(std::forward<decltype(__try_var__)>(__try_var__)); \
    } \
\
    *std::forward<decltype(__try_var__)>(__try_var__); \
})

template<typename... T>
std::nullopt_t extract(std::optional<T...> &&) {
    return std::nullopt;
}

std::optional<int> foo() {
    return 3;
}

std::optional<std::string> bar() {
    auto const &z = TRY(foo());
    auto const &y = TRY(foo());

    return "foo";
}
