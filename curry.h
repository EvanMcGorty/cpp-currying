/*
This is a single-header, powerful and lightweight library that provides support for currying.
It can be found at https://github.com/EvanMcGorty/cpp-currying
It is designed to be simply plopped into a project and included.

Overview of this file:

curry: returns a curried version of a function, in the form of a curried_wrapper

curried: a concept for a curried version of a function, equivalent to the constexpr bool is_curried_v

curry_wrapper<t>::operator(): partially apply any number of arguments to and possibly evaluate a curried function

curry_wrapper<t>::curry_wrapped_val(): access the value wrapped up by curry

curry_wrapper<t>::curry_wrapped_type(): the type returned by curry_wrapped_val()

curry_wrapper<t> default/copy/move constructors, copy/move assignment operators, and a destructor
*/

#pragma once
#include<concepts>
#include<functional>


//Should just be a concept, however it needs to manually handle its variadic arguments
//Rather an implementation detail and can be ignored by the user
//Checks that each argument and the return type are correct when applied one at a time and that target type is an instance of curry_wrapper
//Ideally it would just check the semantics of the class but it is too slow to check every possible way to apply the parameters.
//For example: c(t1,t2,t3,t4), c(t1)(t2,t3,t4), c(t1,t2)(t3)(t4), etc, this grows exponentially with more parameters
template<typename t, typename...ts>
constexpr bool is_curried_v = false //Base case overload, where t doesnt match the form of curry_wraper<something>
	//There is an exception: t might have qualifiers that need to be removed (ie due to being perfect forwarded)
	|| !std::same_as<t,std::remove_cvref_t<t>> && is_curried_v<std::remove_cvref_t<t>,ts...>;

//A concept for functions created by 'curry' and return types of their applications
//No parameters just means that it is any curried function, regardless of parameter types
//Otherwise the first parameter is the return type, and the rest are the argument types
//void can be used to indicate a unit parameter, aka an empty () or invocation with zero arguments
//A curried<A,B,C,D> corresponds to a "B -> C -> D -> A"
//A curried<A> just corresponds to an "A", though still wrapped up by curry, so an implicit conversion is needed
//A std::function<std::function<A(B)>(C)> is curried<A,C,B> as well as curried<std::function<A(B)>,C>
//A std::function<int()> is curried<int,void>
template<typename t, typename...ts>
concept curried = is_curried_v<t, ts...>;

//Curries a function so that it may be called with any number of parameters
//Therefore, it (ironically) also technically functions as an uncurry
//Functions with no parameters are treated as if they take a unit type, and must be explicitly empty-invoked
//If the target callable is passed by (lvalue) reference, then it will be stored by reference (like passing via std::ref to std::bind)
//To have it stored by value, explicitly std::move or copy construct if it isnt already being passed as an rvalue
//Beware: calling curry on an already curried function will simply return it via perfect forwarding
//Applications are done via bind_front, so std::ref and std::cref can be used on arguments to avoid copies.
//Consider std::move-ing your curried callables when calling operator() to avoid copying previous captures into the new callable
//Constness of the resulting object will be propogated to contained values and even references
template<typename callable_t>
constexpr curried auto curry(callable_t&& callable);

//A class to wrap a (possibly) callable object
//Has multiple overloads of operator() for different numbers of parameters
//Has a implicit conversion to the underlying held object
//Rather an implementation detail and can be ignored by the user
template<typename callable_t>
class curry_wrapper
{
private:

	//Make constructor private to prevent ambiguity with the move constructor
	constexpr curry_wrapper(callable_t&& callable) : raw_wrapped_val(std::forward<callable_t>(callable)) {}
	
	//Defer all construction to this function
	template<typename friend_callable_t>
	friend constexpr curried auto curry(friend_callable_t&& callable);

	template<typename friend_callable_t>
	friend class curry_wrapper;

	template<typename forwarding_callable_t, typename arg1_t>
	static constexpr curried auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1)
		requires (std::is_invocable_v<forwarding_callable_t,arg1_t> && !std::is_void_v<std::invoke_result_t<forwarding_callable_t,arg1_t>>)
	{
		return curry(std::invoke(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1)));
	}
	
	template<typename forwarding_callable_t, typename arg1_t>
	static constexpr void do_apply(forwarding_callable_t&& callable, arg1_t&& arg1)
		requires (std::is_void_v<std::invoke_result_t<forwarding_callable_t,arg1_t>>)
	{
		return std::invoke(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1));
	}

	template<typename forwarding_callable_t, typename arg1_t>
	static constexpr curried auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1)
		requires (!std::is_invocable_v<forwarding_callable_t,arg1_t>)
	{
		return curry(std::bind_front(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1)));
	}
	
	template<typename forwarding_callable_t, typename arg1_t, typename...args_t>
	static constexpr curried auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1, args_t&&...args)
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
	static constexpr curried auto do_apply(forwarding_callable_t&& callable)
		requires (!std::is_void_v<std::invoke_result_t<forwarding_callable_t>>)
	{
		return curry(std::invoke(std::forward<forwarding_callable_t>(callable)));
	}

	//kept private to propogate constness through curry_wrapped_val
	callable_t raw_wrapped_val;

	constexpr callable_t&& curry_wrapped_val_or_reference_wrapper() &&
		requires (!std::is_lvalue_reference_v<callable_t>)
	{
		return std::move(raw_wrapped_val);
	}

	constexpr std::reference_wrapper<std::remove_reference_t<callable_t>> curry_wrapped_val_or_reference_wrapper() &&
		requires (std::is_lvalue_reference_v<callable_t>)
	{
		return std::ref(raw_wrapped_val);
	}

	constexpr callable_t const&& curry_wrapped_val_or_reference_wrapper() const&&
		requires (!std::is_lvalue_reference_v<callable_t>)
	{
		return std::move(raw_wrapped_val);
	}

	constexpr std::reference_wrapper<const std::remove_reference_t<callable_t>> curry_wrapped_val_or_reference_wrapper() const&&
		requires (std::is_lvalue_reference_v<callable_t>)
	{
		return std::ref(raw_wrapped_val);
	}

public:

	//The type of the wrapped callable value
	using curry_wrapped_type = callable_t;

	//curry_wrapped_val must propogate constness of the wrapper object to the wrapped object or reference
	//If this is an lvalue then an lvalue should be returned

	constexpr callable_t& curry_wrapped_val() &
	{
		return raw_wrapped_val;
	}
	
	constexpr std::remove_reference_t<callable_t> const& curry_wrapped_val() const&
	{
		auto const& ret = raw_wrapped_val;
		return ret;
	}
	
	constexpr callable_t&& curry_wrapped_val() &&
	{
		return std::move(raw_wrapped_val);
	}
	
	constexpr std::remove_reference_t<callable_t> const& curry_wrapped_val() const&&
		requires (std::is_lvalue_reference_v<callable_t>)
	{
		auto const& ret = raw_wrapped_val;
		return std::move(ret);
	}
	
	constexpr std::remove_reference_t<callable_t> const&& curry_wrapped_val() const&&
		requires (!std::is_lvalue_reference_v<callable_t>)
	{
		auto const& ret = raw_wrapped_val;
		return std::move(ret);
	}
	
	//Implicit conversions to the wrapped value
	//Access the result of using a function through 'curry'
	
	constexpr operator auto() &
	{
		return curry_wrapped_val();
	}
	constexpr operator auto() const&
	{
		return curry_wrapped_val();
	}
	constexpr operator auto() &&
	{
		return std::move(*this).curry_wrapped_val();
	}
	constexpr operator auto() const&&
	{
		return std::move(*this).curry_wrapped_val();
	}


	template<typename...args_t>
	constexpr auto operator()(args_t...args) &
	{
		return do_apply(std::ref(curry_wrapped_val()),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&
	{
		return do_apply(std::ref(curry_wrapped_val()),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) &&
	{
		return do_apply(std::move(*this).curry_wrapped_val_or_reference_wrapper(),std::forward<args_t>(args)...);
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&&
	{
		return do_apply(std::move(*this).curry_wrapped_val_or_reference_wrapper(),std::forward<args_t>(args)...);
	}

	constexpr curry_wrapper() = default;
	constexpr curry_wrapper(curry_wrapper const& rhs) = default;
	constexpr curry_wrapper(curry_wrapper&& rhs) = default;
	constexpr curry_wrapper& operator=(curry_wrapper const& rhs) = default;
	constexpr curry_wrapper& operator=(curry_wrapper&& rhs) = default;
	constexpr ~curry_wrapper() = default;
};

template<typename t>
constexpr bool is_curried_v<curry_wrapper<t>> = true;

template<typename t, typename return_t>
constexpr bool is_curried_v<curry_wrapper<t>, return_t> = std::convertible_to<curry_wrapper<t>,return_t>; //Check return type in a manner consistent with std::is_invocable_r

template<typename t, typename return_t, typename...args_t>
constexpr bool is_curried_v<curry_wrapper<t>, return_t, void, args_t...> = requires (curry_wrapper<t> a)
{
	{a()} -> curried<return_t, args_t...>;
};

template<typename t, typename return_t, typename arg1_t, typename...args_t>
constexpr bool is_curried_v<curry_wrapper<t>, return_t, arg1_t, args_t...> = requires (curry_wrapper<t> a)
{
	{a(std::declval<arg1_t>())} -> curried<return_t, args_t...>;
};

template<typename callable_t>
constexpr curried auto curry(callable_t&& callable)
{
	if constexpr(is_curried_v<callable_t>)
	{
		return std::forward<callable_t>(callable);
	}
	else
	{
		return curry_wrapper<callable_t>(std::forward<callable_t>(callable));
	}
}