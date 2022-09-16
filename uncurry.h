/*
Overview of this file:

uncurry: A function to extract the value wrapped by a curried

uncurried_t: a using template to get the type wrapped by a curried

uncurried: the specialized struct used to implement uncurried_t
*/


#pragma once
#include "curry.h"

template<typename t>
constexpr t& uncurry(curry<t>& f)
{
	return f;
}

template<typename t>
constexpr t const& uncurry(curry<t> const& f)
{
	return f;
}

template<typename t>
constexpr t&& uncurry(curry<t>&& f)
{
	return std::move(f);
}

template<typename t>
constexpr t const&& uncurry(curry<t> const&& f)
{
	return std::move(f);
}

template<typename t>
struct uncurried {};

template<typename t>
struct uncurried<curry<t>>
{
	using type = t;
};

template<typename t>
using uncurried_t = typename uncurried<t>::type;
