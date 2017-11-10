#include <optional>

#define TRY_IMPL(counter, var, expr...) \
    auto &&__try_var##counter = (expr); \
    \
    if(!__try_var##counter) {\
        return extract(std::forward<decltype(__try_var##counter)>(__try_var##counter)); \
    }\
    \
    auto var = *std::forward<decltype(__try_var##counter)>(__try_var##counter)

#define TRY_INDR(counter, var, expr...) \
    TRY_IMPL(counter, var, expr)

#define TRY(var, expr...) \
    TRY_INDR(__COUNTER__, var, expr)

template<typename... T>
std::nullopt_t extract(std::optional<T...> &&) {
    return std::nullopt;
}

std::optional<int> foo() {
    return 3;
}

std::optional<std::string> bar() {
    TRY(const &x, foo());
    TRY(const &y, foo());
    return "foo";
}
