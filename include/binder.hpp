#include <tuple>
#include <iostream>
#include <functional>

template<typename... T>
int foo(T...);

template<typename T>
int foo(T t) {
  return t;
}

template<typename T, typename... U>
int foo(T t, U... u) {
  return t + foo(u...);
}

template<typename... tuples_t>
using tuple_cat_t = decltype(std::tuple_cat(std::declval<tuples_t &&>()...));

template<typename... args_t>
using make_tuple_t = decltype(std::make_tuple(std::declval<args_t &&>()...));
    
template<int, typename = void>
struct get_arg_t;

template<int arg_pos>
struct get_arg_t<arg_pos, std::enable_if_t<(arg_pos > 0)>> {
  template<typename tuple1_t, typename tuple2_t>
  static constexpr inline decltype(auto) _(tuple1_t &&p_args, tuple2_t &&) {
    return std::get<(+arg_pos - 1)>(std::forward<tuple1_t>(p_args));
  }
};

template<int arg_pos>
struct get_arg_t<arg_pos, std::enable_if_t<(arg_pos < 0)>> {
  template<typename tuple1_t, typename tuple2_t>
  static constexpr inline decltype(auto) _(tuple1_t &&, tuple2_t &&p_args) {
    return std::get<(-arg_pos - 1)>(std::forward<tuple2_t>(p_args));
  }
};

template<typename invokeable_t, typename tuple_t, int arg_max, int arg_min, int... arg_pos>
struct binder_t {
  template<typename _tuple_t, int _arg_pos>
  using positional_defer_t = binder_t
    	<invokeable_t, _tuple_t, arg_max, arg_min, arg_pos..., _arg_pos>;

  template<typename _tuple_t>
  using auto_defer_t = binder_t
    	<invokeable_t, _tuple_t, arg_max, arg_min - 1, arg_pos..., arg_min - 1>;
  
  template<typename _tuple_t>
  using bind_t = binder_t
    	<invokeable_t, _tuple_t, arg_max + 1, arg_min, arg_pos..., arg_max + 1>;
  
  invokeable_t m_invokeable;
  tuple_t      m_tuple;
  
  constexpr inline binder_t(invokeable_t p_invokeable, tuple_t p_tuple)
    : m_invokeable(std::move(p_invokeable))
    , m_tuple     (std::move(p_tuple     ))
  { }
  
  constexpr inline auto_defer_t<tuple_t> _() const & {
    return { m_invokeable, m_tuple };
  }
  
  constexpr inline auto_defer_t<tuple_t> _() && {
    return { std::move(m_invokeable), std::move(m_tuple) };
  }
  
  constexpr inline positional_defer_t<tuple_t, -1> _1() const & {
    return { m_invokeable, m_tuple };
  }
  
  constexpr inline positional_defer_t<tuple_t, -1> _1() && {
    return { std::move(m_invokeable), std::move(m_tuple) };
  }
  
  constexpr inline positional_defer_t<tuple_t, -2> _2() const & {
    return { m_invokeable, m_tuple };
  }
  
  constexpr inline positional_defer_t<tuple_t, -2> _2() && {
    return { std::move(m_invokeable), std::move(m_tuple) };
  }
  
  template<typename arg_t>
  constexpr inline bind_t<tuple_cat_t<tuple_t, make_tuple_t<arg_t>>> _(arg_t &&p_arg) const & {
    return { m_invokeable, std::tuple_cat(m_tuple, std::make_tuple(std::forward<arg_t>(p_arg))) };
  }
  
  template<typename arg_t>
  constexpr inline bind_t<tuple_cat_t<tuple_t, make_tuple_t<arg_t>>> _(arg_t &&p_arg) && {
    return { std::move(m_invokeable), std::tuple_cat(std::move(m_tuple), std::make_tuple(std::forward<arg_t>(p_arg))) };
  }
  
  template<typename... args_t>
  constexpr inline decltype(auto) operator ()(args_t&&... p_args) const & {
    auto forward_tuple = std::forward_as_tuple(std::forward<args_t>(p_args)...);
    
    return m_invokeable
      (get_arg_t<arg_pos>::_(m_tuple, std::move(forward_tuple))...);
  }
  
  template<typename... args_t>
  constexpr inline decltype(auto) operator ()(args_t&&... p_args) && {
    auto forward_tuple = std::forward_as_tuple(std::forward<args_t>(p_args)...);
    
    return std::move(m_invokeable)
      (get_arg_t<arg_pos>::_(std::move(m_tuple), std::move(forward_tuple))...);
  }
};

template<typename invokeable_t, typename tuple_t = std::tuple<>>
constexpr inline auto binder(invokeable_t p_invokeable, tuple_t p_tuple = tuple_t()) {
  return binder_t<invokeable_t, tuple_t, 0, 0>(std::move(p_invokeable), std::move(p_tuple));
}

int main() {
#if 1
  auto bound = binder([](auto... x) { return foo(x...); })._(5)._1()._2();
#else
  auto bound = std::bind([](auto... x) { return foo(x...); }, 5, std::placeholders::_1, std::placeholders::_2);
#endif

  std::cout << bound(6, 7) << std::endl;
}
