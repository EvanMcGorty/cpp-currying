/*
This is a single-header, powerful and lightweight library that provides support for currying.
It can be found at https://github.com/EvanMcGorty/cpp-currying
It is designed to be simply plopped into a project and included.
There are also additional optional headers that provide small utilities.

Overview of this file:

curry<t>: class with a constructor from t that wraps a callable and curries it

curry<t>::operator(): partially apply any number of arguments to and possibly evaluate a curried function

curry<t> implicit conversions to references to the wrapped type

curry<t> default/copy/move constructors, copy/move assignment operators, destructor, all created via = default
*/

#pragma once
#include<concepts>
#include<functional>


//Wraps a function so that it may be called with any number of parameters, std::bind_front-ing them one at a time
//Functions with no parameters are treated as if they take a unit type, and must be explicitly empty-invoked
//Use curry<std::reference_wrapper<...>> or construct via curry{std::ref(...)} to capture the callable by reference
//Calling operator() by (lvalue) reference behaves like passing a std::ref of the wrapped callable to std::bind_front
//To have the callable stored by value, explicitly std::move or copy construct if it isnt already being used as an rvalue
//Note the implications of this: when a callable is stored by reference, it can no longer have its rvalue operator() called
//So, be sure to consider std::move-ing your curried callables when calling operator() to avoid unnecessary copying
//Because applications are done via std::bind_front, std::ref and std::cref can be used on arguments to avoid copies.
template<typename callable_t>
class curry
{
private:

	template<typename friend_callable_t>
	friend class curry;

	template<typename ret_t>
	static constexpr auto handle_invoke_result(ret_t&& ret)
	{
		if constexpr(std::is_lvalue_reference_v<ret_t>)
		{
			return ::curry{std::ref(ret)};
		}
		else
		{
			return ::curry{std::move(ret)};
		}
	}

	template<typename forwarding_callable_t, typename arg1_t>
	static constexpr auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1)
		requires (std::is_invocable_v<forwarding_callable_t,arg1_t> && !std::is_void_v<std::invoke_result_t<forwarding_callable_t,arg1_t>>)
	{
		return handle_invoke_result(std::invoke(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1)));
	}

	template<typename forwarding_callable_t, typename arg1_t>
	static constexpr void do_apply(forwarding_callable_t&& callable, arg1_t&& arg1)
		requires (std::is_void_v<std::invoke_result_t<forwarding_callable_t,arg1_t>>)
	{
		return std::invoke(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1));
	}

	template<typename forwarding_callable_t, typename arg1_t>
	static constexpr auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1)
		requires (!std::is_invocable_v<forwarding_callable_t,arg1_t>)
	{
		return ::curry{std::bind_front(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1))};
	}
	
	template<typename forwarding_callable_t, typename arg1_t, typename...args_t>
	static constexpr auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1, args_t&&...args)
	{
		return do_apply(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1))(std::forward<args_t>(args)...);
	}
	
	template<typename forwarding_callable_t>
	static constexpr void do_apply(forwarding_callable_t&& callable)
		requires (std::is_void_v<std::invoke_result_t<forwarding_callable_t>>)
	{
		std::invoke(std::forward<forwarding_callable_t>(callable));
	}

	template<typename forwarding_callable_t>
	static constexpr auto do_apply(forwarding_callable_t&& callable)
		requires (!std::is_void_v<std::invoke_result_t<forwarding_callable_t>>)
	{
		return handle_invoke_result(std::invoke(std::forward<forwarding_callable_t>(callable)));
	}

	//The wrapped value
	callable_t wrapped;

public:

	constexpr curry(callable_t callable) : wrapped(std::move(callable)) {}
	
	constexpr operator callable_t&() &
	{
		return wrapped;
	}
	constexpr operator callable_t const&() const&
	{
		return wrapped;
	}
	constexpr operator callable_t&&() &&
	{
		return std::move(wrapped);
	}
	constexpr operator callable_t const&&() const&&
	{
		return std::move(wrapped);
	}


	template<typename...args_t>
	constexpr auto operator()(args_t...args) &
	{
		return do_apply(std::ref(wrapped),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&
	{
		return do_apply(std::ref(wrapped),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) &&
	{
		return do_apply(std::move(wrapped),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&&
	{
		return do_apply(std::move(wrapped),std::forward<args_t>(args)...);
	}

	constexpr curry() = default;
	constexpr curry(curry const& rhs) = default;
	constexpr curry(curry&& rhs) = default;
	constexpr curry& operator=(curry const& rhs) = default;
	constexpr curry& operator=(curry&& rhs) = default;
	constexpr ~curry() = default;
};