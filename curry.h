/*
This is a single-header, powerful and lightweight library that provides support for currying.
It can be found at https://github.com/EvanMcGorty/cpp-currying
It is designed to be simply plopped into a project and included.
There are also additional optional headers that provide small utilities.

Overview of this file:

curry<t>: the main wrapper class for currying functions

curry<t>::curry: constructor that takes the wrapped type and (optionally) parameters to bind

curry<t>::operator(): partially apply any number of arguments to and possibly evaluate a curried function

curry<t> implicit conversions to references to the wrapped type

curry<t> default/copy/move constructors, copy/move assignment operators, destructor, all created via = default
*/

#pragma once
#include<concepts>
#include<functional>

template<typename callable_t>
class curry;

//Invalid instance of curry, should not occur unless a void results from a curry constructor with multiple arguments
//For this reason, it is used to store some static methods without cluttering the global namespace
//Rather an implementation detail, contents are not meant to be used, though they also dont leak the implementation
//Contents are used by curry<t>, as well as by the deduction guide for its multiple-argument constructor
template<>
class curry<void>
{
public:

	template<typename c_callable_t, typename arg1_t, typename...args_t>
	constexpr curry(c_callable_t&& callable, arg1_t&& arg1, args_t&&...args)
	{
		curry<void>::do_apply(std::forward<c_callable_t>(callable), std::forward<arg1_t>(arg1), std::forward<args_t>(args)...);
	}

	constexpr curry() = delete;
	constexpr curry(curry const& rhs) = delete;
	constexpr curry(curry&& rhs) = delete;
	constexpr curry& operator=(curry const& rhs) = delete;
	constexpr curry& operator=(curry&& rhs) = delete;

	constexpr ~curry() = default;
	

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

	//needed for the deduction guide
	template<typename t>
	struct unwrap_do_apply_result {};

	template<>
	struct unwrap_do_apply_result<void> 
	{
		using type = void;
	};

	template<typename t>
	struct unwrap_do_apply_result<curry<t>>
	{
		using type = t;
	};
};

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

	//The wrapped value
	callable_t wrapped;

public:

	constexpr curry(callable_t callable) : wrapped(std::move(callable)) {}

	template<typename c_callable_t, typename arg1_t, typename...args_t>
	constexpr curry(c_callable_t&& callable, arg1_t&& arg1, args_t&&...args) : 
		curry(curry<void>::do_apply(std::forward<c_callable_t>(callable), std::forward<arg1_t>(arg1), std::forward<args_t>(args)...)) {}
	
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
		return curry<void>::do_apply(std::ref(wrapped),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&
	{
		return curry<void>::do_apply(std::ref(wrapped),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) &&
	{
		return curry<void>::do_apply(std::move(wrapped),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&&
	{
		return curry<void>::do_apply(std::move(wrapped),std::forward<args_t>(args)...);
	}

	constexpr curry() = default;
	constexpr curry(curry const&) = default;
	constexpr curry(curry&&) = default;
	constexpr curry& operator=(curry const&) = default;
	constexpr curry& operator=(curry&&) = default;
	constexpr ~curry() = default;

	template<typename t>
		requires (!std::same_as<callable_t,t>) && std::constructible_from<callable_t, t const&>
	constexpr curry(curry<t> const& rhs) : wrapped(static_cast<t const&>(rhs.wrapped)) {}

	template<typename t>
		requires (!std::same_as<callable_t,t>) && std::constructible_from<callable_t, t&&>
	constexpr curry(curry<t>&& rhs) : wrapped(static_cast<t&&>(std::move(rhs))) {}

	template<typename t>
		requires (!std::same_as<callable_t,t>) && std::assignable_from<callable_t&, t const&>
	constexpr curry& operator=(curry<t> const& rhs) 
	{
		wrapped = static_cast<t&&>(std::move(rhs));
		return *this;
	}

	template<typename t>
		requires (!std::same_as<callable_t,t>) && std::assignable_from<callable_t&, t&&>
	constexpr curry& operator=(curry<t>&& rhs)
	{
		wrapped = static_cast<t&&>(std::move(rhs));
		return *this;
	}

};

template<typename c_callable_t, typename arg1_t, typename...args_t>
curry(c_callable_t&& callable, arg1_t&& arg1, args_t&&...args) ->
	curry<typename curry<void>::unwrap_do_apply_result<decltype(curry<void>::do_apply(std::forward<c_callable_t>(callable), std::forward<arg1_t>(arg1), std::forward<args_t>(args)...))>::type>;