#include <tuple>
#include <cassert>
#include <utility>
#include <iostream>
#include <type_traits>

// TUPLE ELEMENT

template<std::size_t I, std::size_t J, typename T>
struct tuple_element_leaf { };

template<std::size_t I, typename T>
struct tuple_element_leaf<I, I, T> {
    using type = T;
};

template<std::size_t, typename, typename...>
struct tuple_element_base;

template<std::size_t I, std::size_t... J, typename... T>
struct tuple_element_base<I, std::integer_sequence<std::size_t, J...>, T...>
    : public tuple_element_leaf<I, J, T>...
{ };

template<std::size_t I, typename... T>
struct tuple_element
    : private tuple_element_base<I, std::index_sequence_for<T...>, T...>
{
    using type = typename tuple_element_base<I, std::index_sequence_for<T...>, T...>::type;
};

// TUPLE LEAF

template<std::size_t I, typename T, bool = std::is_reference<T>::value, bool = std::is_empty<T>::value>
class tuple_leaf {
  T val;

public:
  template<typename U>
  constexpr tuple_leaf(U&& x) noexcept(noexcept(T(std::forward<U>(x))))
    : val(std::forward<U>(x))
  {}

  T &get() & noexcept {
      return val;
  }

  T &&get() && noexcept {
      return std::move(val);
  }

  constexpr T const &get() const & noexcept {
      return val;
  }
};

template<std::size_t I, typename T, bool B>
class tuple_leaf<I, T, true, B> {
  typename std::remove_reference<T>::type *val;
    
public:
  constexpr tuple_leaf(T x) noexcept
    : val(&x)
  {}

  decltype(auto) get() & noexcept {
    return *val;
  }
  
  decltype(auto) get() && noexcept {
      return std::forward<T>(*val);
  }

  decltype(auto) get() const & noexcept {
    return *val;
  }
};

template<std::size_t I, typename T>
struct tuple_leaf<I, T, false, true> : private T {
    
  template<typename U>
  constexpr tuple_leaf(U &&x) noexcept(noexcept(T(std::forward<U>(x))))
    : T(std::forward<U>(x))
  {}

  decltype(auto) get() & noexcept {
    return *this;
  }

  decltype(auto) get() && noexcept {
    return std::move(*this);
  }

  decltype(auto) get() const & noexcept {
    return *this;
  }
};

// TUPLE BASE

template<typename, typename...>
struct tuple_base;

template<std::size_t... I, typename... T>
struct tuple_base<std::integer_sequence<std::size_t, I...>, T...>
    : private tuple_leaf<I, T>...
{
    template<typename... U>
    constexpr tuple_base(U&&... args)
        : tuple_leaf<I, T>(std::forward<U>(args))...
    { }
    
    template<std::size_t J>
    decltype(auto) get() & noexcept {
        return tuple_leaf<J, typename std::tuple_element<J, std::tuple<T...>>::type>::get();
    }

    template<std::size_t J>
    decltype(auto) get() && noexcept {
        return std::move(*this).tuple_leaf<J, typename std::tuple_element<J, std::tuple<T...>>::type>::get();
    }

    template<std::size_t J>
    decltype(auto) get() const & noexcept {
        return tuple_leaf<J, typename std::tuple_element<J, std::tuple<T...>>::type>::get();
    }
};

// TUPLE

template<typename... T>
struct tuple
    : private tuple_base<std::index_sequence_for<T...>, T...>
{    
    template<typename... U>
    constexpr tuple(U&&... args)
        : tuple_base<std::index_sequence_for<T...>, T...>(std::forward<U>(args)...)
    { }
    
    template<std::size_t I>
    decltype(auto) get() & noexcept {
        return tuple_base<std::index_sequence_for<T...>, T...>::template get<I>();
    }

    template<std::size_t I>
    decltype(auto) get() && noexcept {
        return std::move(*this).tuple_base<std::index_sequence_for<T...>, T...>::template get<I>();
    }

    template<std::size_t I>
    constexpr decltype(auto) get() const & noexcept {
        return tuple_base<std::index_sequence_for<T...>, T...>::template get<I>();
    }
};

struct blah_t { };

int main() {
    int k = 2;
    
    auto foo = tuple<int, int, blah_t, int &>(1, 0, blah_t(), k);
    
    int &i = foo.get<0>();
    int &j = foo.get<1>();
    int &l = foo.get<3>();
    
    std::cout << i << std::endl;
    std::cout << j << std::endl;
    std::cout << l << std::endl;
    
    std::cout << sizeof(foo) << std::endl;
}
