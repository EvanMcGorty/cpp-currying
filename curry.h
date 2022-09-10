#pragma once
#include<concepts>
#include<functional>

template<typename callable_t>
constexpr auto curry(callable_t&& callable);

//A class to wrap a (possibly) callable object
//Has multiple overloads of operator() for different numbers of parameters
//Has a implicit conversion to the underlying held object
//Rather an implementation detail and can be ignored by the user
//It is meaningless for this class to be const, other than for making copies
template<typename callable_t>
class curry_wrapper
{
private:

	//Make constructor private to prevent ambiguity with the move constructor
	constexpr curry_wrapper(callable_t&& callable) : curry_wrapped_val(std::forward<callable_t>(callable)) {}
	
	//Defer all construction to this function
	template<typename friend_callable_t>
	friend constexpr auto curry(friend_callable_t&& callable);

	template<typename friend_callable_t>
	friend class curry_wrapper;

	//Try invoking with no parameters, otherwise return a reference to *this
	//Used to force evaluation after bind_front-ing the last argument of a function
	//(but only if it was indeed the last argument)
	constexpr auto try_empty_invoke()
		requires std::is_invocable_v<callable_t>
	{
		return (*this)();
	}

	constexpr curry_wrapper& try_empty_invoke()
		requires (!std::is_invocable_v<callable_t>)
	{
		return *this;
	}

	template<typename forwarding_callable_t, typename arg1_t>
	static constexpr auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1)
	{
		auto&& res = curry(std::bind_front(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1)));
		return res.try_empty_invoke();
	}
	
	template<typename forwarding_callable_t, typename arg1_t, typename...args_t>
	static constexpr auto do_apply(forwarding_callable_t&& callable, arg1_t&& arg1, args_t&&...args)
	{
		return do_apply(std::forward<forwarding_callable_t>(callable),std::forward<arg1_t>(arg1))(std::forward<args_t>(args)...);
	}
	
	template<typename forwarding_callable_t>
	static constexpr auto do_apply(forwarding_callable_t&& callable)
	{
		if constexpr(std::is_void_v<decltype(std::invoke(std::forward<forwarding_callable_t>(callable)))>)
		{
			std::invoke(std::forward<forwarding_callable_t>(callable));
		}
		else
		{
			return curry(std::invoke(std::forward<forwarding_callable_t>(callable)));
		}
	}

public:

	//The type of the wrapped callable value
	using curry_wrapped_type = callable_t;

	//The wrapped callable value
	curry_wrapped_type curry_wrapped_val;

	//Implicit conversions to the wrapped value
	//Access the result of using a function through 'curry'
	
	constexpr operator callable_t&() &
	{
		return curry_wrapped_val;
	}
	constexpr operator callable_t const&() const&
	{
		return curry_wrapped_val;
	}
	constexpr operator callable_t&&() &&
	{
		return std::move(curry_wrapped_val);
	}
	constexpr operator callable_t const&() const&&
	{
		return std::move(curry_wrapped_val);
	}

	//todo: as_ref, as_const_ref, as_rvalue_ref, as_val
	//maybe curry_by_ref, rvref, val, cref, crvref, cval
	//maybe just use constructors to handle this somehow?


	template<typename...args_t>
	constexpr auto operator()(args_t...args) &
	{
		//Make a clone unless callable_t itself also happens to be a reference type
		if constexpr (std::is_lvalue_reference_v<callable_t>)
		{
			return do_apply(std::ref(curry_wrapped_val),std::forward<args_t>(args)...);
		}
		//Probably rare case
		else if constexpr (std::is_rvalue_reference_v<callable_t>)
		{
			//std::move (will end up calling move constructor)
			return do_apply(std::move(curry_wrapped_val),std::forward<args_t>(args)...);
		}
		else
		{
			return do_apply(callable_t{curry_wrapped_val},std::forward<args_t>(args)...);
		}
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&
	{
		//Make a clone unless callable_t itself also happens to be a reference type
		if constexpr (std::is_lvalue_reference_v<callable_t>)
		{
			return do_apply(std::ref(curry_wrapped_val),std::forward<args_t>(args)...);
		}
		//Probably rare case
		else if constexpr (std::is_rvalue_reference_v<callable_t>)
		{
			//std::move (will end up calling const&&/copy constructor)
			//Probably not very necessary but logically correct
			return do_apply(std::move(curry_wrapped_val),std::forward<args_t>(args)...);
		}
		else
		{
			return do_apply(callable_t{curry_wrapped_val},std::forward<args_t>(args)...);
		}
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) &&
	{
		//std::move (will end up calling the move constructor) unless callable_t itself happens to be a reference type
		if constexpr (std::is_lvalue_reference_v<callable_t>)
		{
			return do_apply(std::ref(curry_wrapped_val),std::forward<args_t>(args)...);
		}
		else
		{
			return do_apply(std::move(curry_wrapped_val),std::forward<args_t>(args)...);
		}
	}

	template<typename...args_t>
	constexpr auto operator()(args_t...args) const&&
	{
		//std::move (will end up calling const&&/copy constructor) unless callable_t itself happens to be a reference type
		if constexpr (std::is_lvalue_reference_v<callable_t>)
		{
			return do_apply(std::ref(curry_wrapped_val),std::forward<args_t>(args)...);
		}
		else
		{
			return do_apply(std::move(curry_wrapped_val),std::forward<args_t>(args)...);
		}
	}

	constexpr curry_wrapper() = default;
	constexpr curry_wrapper(curry_wrapper const& rhs) = default;
	constexpr curry_wrapper(curry_wrapper&& rhs) = default;
	constexpr curry_wrapper& operator=(curry_wrapper const& rhs) = default;
	constexpr curry_wrapper& operator=(curry_wrapper&& rhs) = default;
	constexpr ~curry_wrapper() = default;
};

//Should just be a concept, however it needs to manually handle its variadic arguments
//Rather an implementation detail and can be ignored by the user
template<typename t, typename...ts>
constexpr bool is_curried_v = false;

//Predeclare 'curried' in order to use concept syntax in the other definitions of is_curried_v
//Strictly speaking not necessary

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

template<typename t>
constexpr bool is_curried_v<curry_wrapper<t>> = true;

template<typename t, typename return_t>
constexpr bool is_curried_v<curry_wrapper<t>, return_t> = std::same_as<t,return_t>;  //and implicitly, std::convertible_to<curry_wrapper<t>,return_t>

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

//Curries a function so that it may be called with any number of parameters
//Therefore, it also technically functions as an uncurry
//Functions with no parameters are treated as if they take a unit type, and must be explicitly empty-invoked
//If the target callable is passed by (lvalue) reference, then it will be stored by reference
//You can call curry on an already curried function to change its qualifiers
//Applications are done via bind_front, so std::ref and std::cref can be used to avoid copies.
//Consider std::move-ing your curried callables when calling operator() to avoid copying previous captures into the new callable
template<typename callable_t>
constexpr auto curry(callable_t&& callable)
{
	if constexpr(is_curried_v<std::remove_cvref_t<callable_t>>)
	{
		return std::forward<callable_t>(callable);
	}
	else
	{
		return curry_wrapper<callable_t>(std::forward<callable_t>(callable));
	}
}
