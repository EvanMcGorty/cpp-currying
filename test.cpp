#include<iostream>
#include"curry.h"

auto expr(int a, int b)
{
	std::cout << "expr has been evaluated\n";

	return std::function{[=](int c, int d) //std::function is unnecessary but will be used to demonstrate something in main
	{
		std::cout << "lambda inside expr has been evaluated\n";
		return a + b + c + d;
	}};
}

void test_function(curried<int,int,int,int,int> auto f)
{
	std::cout << "applying (1)\n";
	auto f1 = f(1);

	std::cout << "applying (2,4)\n";
	auto f124 = f1(2,4);

	std::cout << "applying (8)\n";
	auto f1248 = f124(8);

	std::cout << "printing result:\n";
	std::cout << f1248 << "\n\n";

	std::cout << "printing curry(expr)(1,2,4,8):\n";
	std::cout << f(1,2,4,8) << "\n\n";

	std::cout << "printing curry(expr)(1)(2)(4)(8):\n";
	std::cout << f(1)(2)(4)(8) << "\n\n";
}

int main()
{
	auto cexpr = curry(expr);

	test_function(cexpr);

	std::cout << curry([](){std::cout << "curry(function with no arguments)() = "; return 100;})() << "\n\n";

	curry([](auto s){std::cout << s << "\n\n";})("curry(function with no return type)");

	std::cout << "curry(\"a raw value\") = " << '"' << curry("a raw value") << '"' << "\n\n";

	std::cout << "don't " << curry([](){
		std::cout << "forget "; return [](int a){
			std::cout << "your "; return [=](int b){
				std::cout << "unit "; return [=](){
					std::cout << "applications: "; return a + b;};};};})
		()(1,2)() << "\n\n";

	std::function<int(int,int)> f2 = curry(expr)(1)(2); //does *not* construct a new std::function

	char c; std::cin >> c;
}