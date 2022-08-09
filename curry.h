#include<concepts>
#include<functional>

template<typename callable_t>
constexpr auto curry(callable_t&& callable);

template<typename callable_t>
class curried
{
private:

	callable_t callable;

	constexpr curried(callable_t&& callable) : callable(std::forward<callable_t>(callable)) {}
	
	template<typename fcallable_t>
	friend constexpr auto curry(fcallable_t&& callable);

	template<typename fcallable_t>
	friend class curried;

	constexpr auto try_empty_invoke()
	{
		if constexpr(std::is_invocable_v<callable_t>)
		{
			return (*this)();
		}
		else
		{
			return curry(std::forward<callable_t>(callable));
		}
	}

public:

	constexpr operator callable_t()
	{
		return std::forward<callable_t>(callable);
	}

	template<typename arg1_t,typename...args_t>
	constexpr auto operator()(arg1_t arg1, args_t...args)
	{
		return curry(std::bind_front(std::forward<callable_t>(callable),std::forward<arg1_t>(arg1))).try_empty_invoke()(std::forward<args_t>(args)...);
	}

	template<typename arg1_t>
	constexpr auto operator()(arg1_t arg1)
	{
		return curry(std::bind_front(std::forward<callable_t>(callable),std::forward<arg1_t>(arg1))).try_empty_invoke();
	}

	constexpr auto operator()()
	{
		if constexpr(std::is_void_v<decltype(std::invoke(std::forward<callable_t>(callable)))>)
		{
			std::invoke(std::forward<callable_t>(callable));
		}
		else
		{
			return curry(std::invoke(std::forward<callable_t>(callable)));
		}
	}

	constexpr curried() = default;
	constexpr curried(curried const& rhs) = default;
	constexpr curried(curried&& rhs) = default;
	constexpr curried& operator=(curried const& rhs) = default;
	constexpr curried& operator=(curried&& rhs) = default;
	constexpr ~curried() = default;
};

//curries a function so that it may be called with any number of parameters
template<typename callable_t>
constexpr auto curry(callable_t&& callable)
{
	return curried<callable_t>(std::forward<callable_t>(callable));
}