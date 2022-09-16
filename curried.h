/*
Overview of this file:

curried: a concept for instances of curried

is_curried_v: The constexpr bool used to implement curried

is_curried: A template struct with a static constexpr bool value corresponding to is_curried_v
*/


#pragma once
#include "curry.h"


//Should just be a concept, however it needs to manually handle its variadic arguments
//Rather an implementation detail and can be ignored by the user
//Checks that each argument and the return type are correct when applied one at a time and that target type is an instance of curry
//Ideally it would just check the semantics of the class but it is too slow to check every possible way to apply the parameters.
//For example: c(t1,t2,t3,t4), c(t1)(t2,t3,t4), c(t1,t2)(t3)(t4), etc, and this gets exponentially worse with more parameters
template<typename t, typename...ts>
constexpr bool is_curried_v = false; //Base case overload, where t doesnt match the form of curry_wraper<something>

//is_curried<t,ts...>::value is equivalent to is_curried_v<t,ts...>
//Unnecessary, only exists to be consistent with the style of standard library type_traits
template<typename t, typename...ts>
struct is_curried
{
	constexpr static bool value = is_curried_v<t,ts...>;
};

//curried<t> might have qualifiers obscuring it (ie due to being perfect forwarded)
//Also, a reference wrapper does variadic perfect forwarding on its operator() to the wrapped curried type
//This actually means that their semantics satisfy all of the laws of curried types (as defined above)
//Feel free to overload is_curried_v if your own types properly wrap a curried type

template<typename t, typename...ts>
constexpr bool is_curried_v<t const, ts...> = is_curried_v<t, ts...>;
template<typename t, typename...ts>
constexpr bool is_curried_v<t volatile, ts...> = is_curried_v<t, ts...>;
template<typename t, typename...ts>
constexpr bool is_curried_v<t&, ts...> = is_curried_v<t, ts...>;
template<typename t, typename...ts>
constexpr bool is_curried_v<t&&, ts...> = is_curried_v<t, ts...>;

//require the reference wrapper to directly contain the curry<t>, so that multiple layers of such wrapper types are disallowed
template<typename t, typename...ts>
constexpr bool is_curried_v<std::reference_wrapper<curry<t>>, ts...> = is_curried_v<curry<t>, ts...>;

//A concept for instances of curry (and references/reference_wrappers thereof) and the types of their call operators
//No parameters just means that it is any curried function, regardless of return/parameter types
//Otherwise the first parameter is the return type, and the rest are the argument types
//void can be used to indicate a unit parameter, aka an empty () or invocation with zero arguments
//A curried<A,B,C,D> corresponds to a "B -> C -> D -> A"
//A curried<A> just corresponds to an "A", though still wrapped up by curry, so an implicit conversion is needed
//A curry<std::function<std::function<A(B)>(C)>> is curried<A,C,B> as well as curried<std::function<A(B)>,C>
//A curry<std::function<int()>> is curried<int,void>
//Note: (curried<myfn_t,ret_t,args_t...>) if and only if (curried<myfn_t> && std::is_invokable_r_v<ret_t,myfn_t,args_t...>)
template<typename t, typename...ts>
concept curried = is_curried_v<t, ts...>;

//Generic check that type is an instance of curry, independent of return type or arguments
template<typename t>
constexpr bool is_curried_v<curry<t>> = true;

//Base case where the type is a void, presumably resulting from a curried<void,t>
template<>
constexpr bool is_curried_v<void, void> = true; //Because a curried<void,t> applied to t returns a void (and not a curried<void>)

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