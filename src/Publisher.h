#ifndef PUBLISHER_H
#define PUBLISHER_H

// System Headers
#include <functional>
#include <list>
#include <memory>
#include <type_traits>

#ifdef __cpp_lib_integer_sequence
#include <utility>
#endif

namespace tjh
{
namespace detail
{
  //! The parameter pack object that will serve to conjure dynamic replacements
  //! to std::placeholders:_X
  template<int T>
  struct placeholder{};

} // end detail
} // end tjh

namespace std
{
  //! A necessary foray into the std namespace to denote that our placeholder
  //! object can be used inplace of a std::placeholder
  template<int N>
  struct is_placeholder<tjh::detail::placeholder<N>> : std::integral_constant<int, N>
  {
    // Empty Implementation
  };

} // std


//! This is simply pulling in the integer sequence components that exist in
//! C++14. If C++14 isn't enabled, they are implemented below in C++11.
#ifndef __cpp_lib_integer_sequence

// Taken from libstdc++-v3
// Revision: 210470

namespace tjh
{
namespace std_impl
{
  // Stores a tuple of indices. Used by tuple and pair, and by bind() to
  // extract the elements in a tuple.
  template<size_t... _Indexes>
  struct _Index_tuple
  {
    typedef _Index_tuple<_Indexes..., sizeof...(_Indexes)> __next;
  };

  // Builds an _Index_tuple<0, 1, 2, ..., _Num-1>.
  template<size_t _Num>
  struct _Build_index_tuple
  {
    typedef typename _Build_index_tuple<_Num - 1>::__type::__next __type;
  };

  template<>
  struct _Build_index_tuple<0>
  {
    typedef _Index_tuple<> __type;
  };

  /// Class template integer_sequence
  template<typename _Tp, _Tp... _Idx>
  struct integer_sequence
  {
      typedef _Tp value_type;
      static constexpr size_t size() { return sizeof...(_Idx); }
  };

  template<typename _Tp, _Tp _Num, typename _ISeq = typename _Build_index_tuple<_Num>::__type>
  struct _Make_integer_sequence;

  template<typename _Tp, _Tp _Num, size_t... _Idx>
  struct _Make_integer_sequence<_Tp, _Num, _Index_tuple<_Idx...>>
  {
    static_assert( _Num >= 0, "Cannot make integer sequence of negative length" );

    typedef integer_sequence<_Tp, static_cast<_Tp>(_Idx)...> __type;
  };

  /// Alias template make_integer_sequence
  template<typename _Tp, _Tp _Num>
  using make_integer_sequence = typename _Make_integer_sequence<_Tp, _Num>::__type;

  /// Alias template index_sequence
  template<size_t... _Idx>
  using index_sequence = integer_sequence<size_t, _Idx...>;

  /// Alias template make_index_sequence
  template<size_t _Num>
  using make_index_sequence = make_integer_sequence<size_t, _Num>;

} // std_impl
} // tjh

//! Add the necessary sequences to the std namespace to support future
//! compatibility with C++14.
namespace std
{
  /// Alias template integer_sequence
  template<typename _Tp, _Tp... _Idx>
  using integer_sequence = tjh::std_impl::integer_sequence<_Tp, _Idx...>;

  /// Alias template make_integer_sequence
  template<typename _Tp, _Tp _Num>
  using make_integer_sequence = tjh::std_impl::make_integer_sequence<_Tp, _Num>;

  /// Alias template index_sequence
  template<size_t... _Idx>
  using index_sequence = tjh::std_impl::integer_sequence<size_t, _Idx...>;

  /// Alias template make_index_sequence
  template<size_t _Num>
  using make_index_sequence = tjh::std_impl::make_integer_sequence<size_t, _Num>;

}// end std

#endif

namespace tjh
{
  //! A generic publisher interface which implements an observer pattern.
  //! @tparam T The argument return type of the published event.
  template<typename... T>
  struct IPublisher
  {
  public:
    virtual void subscribe(std::weak_ptr<std::function<void(T...)> > callback) = 0;
    virtual ~IPublisher() {}
  };

  //! A concrete implementation of an IPublisher which exposes a "fire" method
  //! to an owner through a templated friend.
  //! @tparam Friend The class which owns the instance.
  //! @tparam @see IPublisher.
  template <class Friend, typename... T>
  class Publisher : public IPublisher<T...>
  {
  public:
    //! The registration method for attaching a listener to an observer.
    //! @param callback The callback function which should be raised to the
    //!        listener when fired.
    //! @note There is no unsubscribe, the client should just null the weak_ptr
    virtual void subscribe(std::weak_ptr<std::function<void(T...)> > callback)
    {
      if (!callback.expired())
      {
        _callbacks.push_back(callback);
      }
    }

    ~Publisher()
    {
      // Empty Implementation
    }

  protected:
    friend Friend;
    //! A method used to propagate an object of type T to any listeners.
    //! @param args The argument to pass to any listeners.
    virtual void fire(T... args)
    {
      // Iterate through the callback list and remove stale pointers
      auto callback = std::begin(_callbacks);
      while (callback != std::end(_callbacks))
      {
        if (callback->expired())
        {
          callback = _callbacks.erase(callback);
        }
        else
        {
          ++callback;
        }
      }

      // Copy the list to enforce the construct that you cannot add a subscriber
      // while the event is firing as the ordering could be ambiguous and is
      // therefore not supported.
      auto callbacks = _callbacks;

      for (callback = std::begin(callbacks);
        callback != std::end(callbacks);
        ++callback)
      {
        if (auto ptr = callback->lock())
        {
          ptr->operator()(args...);
        }
      }
    }

    //! The callback list which contains weak_ptrs to listener callbacks
    std::list<std::weak_ptr<std::function<void(T...)> > > _callbacks;

  }; // end Publisher

  namespace detail
  {
    //! A help method that converts a sequence parameter pack into a
    //! placeholder parameter pack. This method directly calls std::bind with
    //! the appropriate number of placeholders.
    //! @param mem_func The instance member function pointer
    //! @param obj The instance object
    template<typename Instance, typename Return, typename... Args, int... Indices>
    auto bind_impl( Return(Instance::*mem_func)(Args...), Instance* obj, std::integer_sequence<int, Indices...>) ->
        decltype(std::bind(mem_func,obj,  placeholder<Indices+1>{}...))
    {
      return std::bind(mem_func, obj, placeholder<Indices+1>{}...);
    }

    //! std::shared_ptr overload
    //! @see bind_impl
    template<typename Instance, typename Return, typename... Args, int... Indices>
    auto bind_impl(
      Return(Instance::*mem_func)(Args...),
      std::shared_ptr<Instance> obj,
      std::integer_sequence<int, Indices...>) ->
      decltype(std::bind(mem_func,obj,  placeholder<Indices+1>{}...))
    {
      return std::bind(mem_func, obj, placeholder<Indices+1>{}...);
    }

    //! Free function overload
    //! @see bind_impl
    template<typename Return, typename... Args, int... Indices>
    auto bind_impl(
      Return(*func)(Args...),
      std::integer_sequence<int, Indices...>) ->
      decltype(std::bind(func, placeholder<Indices+1>{}...))
    {
      return std::bind(func, placeholder<Indices+1>{}...);
    }

    //! A helper method that servers to conjure placeholders based on the
    //! number of arguments. The number of arguments are forwarded as a
    //! variadic sequence.
    //! @param mem_func The instance member function pointer
    //! @param obj The instance object
    template<typename Instance, typename Return, typename... Args>
    auto bind_impl(Return (Instance::*mem_func)(Args...), Instance* obj) ->
      decltype(bind_impl(mem_func, obj, std::make_integer_sequence<int, sizeof...(Args)>()))
    {
      return
        bind_impl(
          mem_func,
          obj,
          std::make_integer_sequence<int, sizeof...(Args)>{});
    }

    //! std::shared_ptr overload
    //! @see bind_impl
    template<typename Instance, typename Return, typename... Args>
    auto bind_impl(
      Return (Instance::*mem_func)(Args...),
      std::shared_ptr<Instance> obj) ->
      decltype(bind_impl(mem_func, obj, std::make_integer_sequence<int, sizeof...(Args)>()))
    {
      return
        bind_impl(
          mem_func,
          obj,
          std::make_integer_sequence<int, sizeof...(Args)>{});
    }

    //! Free function overload
    //! @see bind_impl
    template<typename Return, typename... Args>
    auto bind_impl(Return(*func)(Args...)) ->
        decltype(bind_impl(func, std::make_integer_sequence<int, sizeof...(Args)>()))
    {
      return
        bind_impl(
          func,
          std::make_integer_sequence<int, sizeof...(Args)>{});
    }

  } // end detail

  //! A helper method to produce callback objects suitable for registration
  //! with a tjh::Publisher that are tied to instance objects
  //! @param mem_func The instance member function pointer
  //! @param obj The instance object
  template <typename Instance, typename Return, typename... Args>
  std::shared_ptr<std::function<Return(Args...)>>
    make_callback(Return(Instance::*mem_func)(Args...), Instance* obj)
  {
    auto f = detail::bind_impl(mem_func, obj);

    return std::shared_ptr<std::function<Return(Args...)>>(
      new std::function<Return(Args...)>(f));
  }

  //! std::shared_ptr overload
  //! @see make_callback
  template <typename Instance, typename Return, typename... Args>
  std::shared_ptr<std::function<Return(Args...)>>
    make_callback(Return(Instance::*mem_func)(Args...), std::shared_ptr<Instance> obj)
  {
    auto f = detail::bind_impl(mem_func, obj);

    return std::shared_ptr<std::function<Return(Args...)>>(
      new std::function<Return(Args...)>(f));
  }

  //! A helper method to produce callback objects suitable for registration
  //! with a tjh::Publisher that are tied to free methods
  //! @param func The free function pointer
  template <typename Return, typename... Args>
  std::shared_ptr<std::function<Return(Args...)>>
    make_callback(Return(*func)(Args...))
  {
    auto f = detail::bind_impl(func);

    return std::shared_ptr<std::function<Return(Args...)>>(
      new std::function<Return(Args...)>(f));
  }

} // end tjh

#endif // PUBLISHER_H
