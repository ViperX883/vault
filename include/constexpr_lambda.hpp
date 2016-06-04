#include <string>
#include <utility>
#include <iostream>

template<class T>
struct static_constexpr_storage {
  static constexpr T value = T();
};

template<class T>
constexpr T static_constexpr_storage<T>::value;

template<class T>
constexpr const T& static_constexpr() {
  return static_constexpr_storage<T>::value;
}

template<typename F>
struct function_adaptor_t {
  constexpr inline function_adaptor_t() noexcept { }
    
  template<typename... args>
  constexpr inline auto operator ()(args&&... p_args) const ->
    decltype((*reinterpret_cast<F const *>(this))(std::forward<args>(p_args)...))
  {
    return (*reinterpret_cast<F const *>(this))(std::forward<args>(p_args)...);
  }
  
  static_assert(std::is_empty<F>::value, "");
};

#define CONST_FOLD(x...) (__builtin_constant_p(x) ? (x) : (x))

template<typename T>
struct function_adaptor_factory_t {
  constexpr inline function_adaptor_factory_t() noexcept { }
    
  template<typename F>
  constexpr inline function_adaptor_t<F> const &operator += (F*) const noexcept {
    return CONST_FOLD(reinterpret_cast<function_adaptor_t<F> const &>(static_constexpr<T>()));
  }
};

struct addressof_helper_t {
  constexpr inline addressof_helper_t() noexcept { }
  
  template<typename T>
  constexpr inline std::remove_reference_t<T> *operator = (T&&) const noexcept {
    return nullptr;
  }
};

template<std::size_t N>
struct overload_choice_t : public overload_choice_t<N + 1> {
    constexpr inline overload_choice_t() noexcept { }
};

template<>
struct overload_choice_t<30> {
    constexpr inline overload_choice_t() noexcept { }
};

struct select_overload_t : public overload_choice_t<0> {
  constexpr inline select_overload_t() noexcept { }
};

template<std::size_t, typename... F>
struct conditional_impl_t;

template<std::size_t OverloadRank, typename F, typename... TailF>
struct conditional_impl_t<OverloadRank, F, TailF...>
  : private conditional_impl_t<OverloadRank+1, TailF...>
{
  constexpr inline conditional_impl_t() noexcept { }
  
  template<typename... Args>
  constexpr inline auto operator ()(overload_choice_t<OverloadRank>, Args&&... p_args) const ->
    decltype(function_adaptor_t<F>()(std::forward<Args>(p_args)...))
  {
    return function_adaptor_t<F>()(std::forward<Args>(p_args)...);
  }

  using conditional_impl_t<OverloadRank+1, TailF...>::operator ();
};

template<std::size_t OverloadRank, typename F>
struct conditional_impl_t<OverloadRank, F> {
  constexpr inline conditional_impl_t() noexcept { }
  
  template<typename... Args>
  constexpr inline auto operator ()(overload_choice_t<OverloadRank>, Args&&... p_args) const ->
    decltype(function_adaptor_t<F>()(std::forward<Args>(p_args)...))
  {
    return function_adaptor_t<F>()(std::forward<Args>(p_args)...);
  }
};

template<typename... F>
struct conditional_t : private conditional_impl_t<0, F...> {
  constexpr inline conditional_t() noexcept { }
  
  template<typename... Args>
  constexpr inline decltype(auto) operator ()(Args&&... p_args) const {
    return conditional_impl_t<0, F...>::operator ()(select_overload_t(), std::forward<Args>(p_args)...);
  }
};

struct ___constexpr_fn_t { };

#define CONSTEXPR_FUNCTION(x) \
  static constexpr auto const &x = function_adaptor_factory_t<___constexpr_fn_t>() += true ? nullptr : addressof_helper_t()

#define CONSTEXPR_LAMBDA(x...) \
  function_adaptor_factory_t<___constexpr_fn_t>() += true ? nullptr : addressof_helper_t() = []

CONSTEXPR_FUNCTION(conditional) = [](auto&&... f) {
  return conditional_t<std::remove_reference_t<decltype(f)>...> { };
};

CONSTEXPR_FUNCTION(bar) = []() {
  std::cout << "Bar" << std::endl;
};

CONSTEXPR_FUNCTION(foo) = []() {
    std::cout << "Foo" << std::endl;
};

#define ENABLE_IF(x...) std::enable_if_t<(x()()), int> = 5

CONSTEXPR_FUNCTION(print_type) = conditional(
  [](auto i, ENABLE_IF(std::is_same       <decltype(i), int >)) { std::cout << "int"  << std::endl; },
  [](auto i, ENABLE_IF(std::is_convertible<decltype(i), long>)) { std::cout << "long" << std::endl; }
);

#define MEMBER_FUNCTION_LAMBDA(x) [](auto &&arg) -> \
  decltype(std::forward<decltype(arg)>(arg).x()) \
{ \
  return std::forward<decltype(arg)>(arg).x(); \
}

#define FREE_FUNCTION_LAMBDA(x) [](auto &&arg) -> \
  decltype(x(std::forward<decltype(arg)>(arg))) \
{ \
  return x(std::forward<decltype(arg)>(arg)); \
}

CONSTEXPR_FUNCTION(begin) = conditional(
  MEMBER_FUNCTION_LAMBDA(     begin),
    FREE_FUNCTION_LAMBDA(     begin),
    FREE_FUNCTION_LAMBDA(std::begin)
);

int main() {
  foo();
  bar();
  
  print_type((int)      5);
  print_type((long)     5);
  print_type((unsigned) 5);
  
  std::string blah;
  
  auto itr = begin(blah);
  
  int x[] = { 1, 2, 4};
  
  auto aff = begin(x);
}
