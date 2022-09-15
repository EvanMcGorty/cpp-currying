/*
This is a single-header, powerful and lightweight library that provides support for currying.
It can be found at https://github.com/EvanMcGorty/cpp-currying
It is designed to be simply plopped into a project and included.

Overview of this file:

curry<t>: class with a constructor from t that wraps a callable and curries it

curry<t>::operator(): partially apply any number of arguments to and possibly evaluate a curried function

curry<t>::curry_wrapped_val: the wrapped value

curry<t>::curry_wrapped_type: the type of curry_wrapped_val, same as 't'

curry<t> implicit conversions to references to curry_wrapped_val

curry<t> default/copy/move constructors, copy/move assignment operators, destructor

curried: a concept for a curried version of a function, equivalent to the constexpr bool is_curried_v
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

public:

	constexpr curry(callable_t callable) : curry_wrapped_val(std::move(callable)) {}

	//The type of the wrapped value
	using curry_wrapped_type = callable_t;

	//The wrapped value
	curry_wrapped_type curry_wrapped_val;
	
	constexpr operator curry_wrapped_type&() &
	{
		return curry_wrapped_val;
	}
	constexpr operator curry_wrapped_type const&() const&
	{
		return curry_wrapped_val;
	}
	constexpr operator curry_wrapped_type&&() &&
	{
		return std::move(curry_wrapped_val);
	}
	constexpr operator curry_wrapped_type const&&() const&&
	{
		return std::move(curry_wrapped_val);
	}


	template<typename...args_t>
	constexpr auto operator()(args_t...args) &
	{
		return do_apply(std::ref(curry_wrapped_val),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&
	{
		return do_apply(std::ref(curry_wrapped_val),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) &&
	{
		return do_apply(std::move(curry_wrapped_val),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&&
	{
		return do_apply(std::move(curry_wrapped_val),std::forward<args_t>(args)...);
	}

	constexpr curry() = default;
	constexpr curry(curry const& rhs) = default;
	constexpr curry(curry&& rhs) = default;
	constexpr curry& operator=(curry const& rhs) = default;
	constexpr curry& operator=(curry&& rhs) = default;
	constexpr ~curry() = default;
};


//Should just be a concept, however it needs to manually handle its variadic arguments
//Rather an implementation detail and can be ignored by the user
//Checks that each argument and the return type are correct when applied one at a time and that target type is an instance of curry
//Ideally it would just check the semantics of the class but it is too slow to check every possible way to apply the parameters.
//For example: c(t1,t2,t3,t4), c(t1)(t2,t3,t4), c(t1,t2)(t3)(t4), etc, and this gets exponentially worse with more parameters
template<typename t, typename...ts>
constexpr bool is_curried_v = false //Base case overload, where t doesnt match the form of curry_wraper<something>
	//There is an exception: t might have qualifiers that need to be removed (ie due to being perfect forwarded)
	|| !std::same_as<t,std::remove_cvref_t<t>> && is_curried_v<std::remove_cvref_t<t>,ts...>;

//A concept for instances of curry and the types of their applications
//No parameters just means that it is any curried function, regardless of parameter types
//Otherwise the first parameter is the return type, and the rest are the argument types
//void can be used to indicate a unit parameter, aka an empty () or invocation with zero arguments
//A curried<A,B,C,D> corresponds to a "B -> C -> D -> A"
//A curried<A> just corresponds to an "A", though still wrapped up by curry, so an implicit conversion is needed
//A std::function<std::function<A(B)>(C)> is curried<A,C,B> as well as curried<std::function<A(B)>,C>
//A std::function<int()> is curried<int,void>
template<typename t, typename...ts>
concept curried = is_curried_v<t, ts...>;

//Generic check that type is an instance of curry, independent of return type or arguments
template<typename t>
constexpr bool is_curried_v<curry<t>> = true;

//Base case: check if something is simply a particular type wrapped up by curry
template<typename t, typename return_t>
constexpr bool is_curried_v<curry<t>, return_t> = std::convertible_to<curry<t>,return_t>; //Check return type in a manner consistent with std::is_invocable_r

//Recursive case where first argment is a unit/empty application (confusingly represented as the void type)
template<typename t, typename return_t, typename...args_t>
constexpr bool is_curried_v<curry<t>, return_t, void, args_t...> = requires (curry<t> a)
{
	{a()} -> curried<return_t, args_t...>;
};

//Recursive case where first argment is a particular type (confusingly represented as the void type)
template<typename t, typename return_t, typename arg1_t, typename...args_t>
constexpr bool is_curried_v<curry<t>, return_t, arg1_t, args_t...> = requires (curry<t> a)
{
	{a(std::declval<arg1_t>())} -> curried<return_t, args_t...>;
};