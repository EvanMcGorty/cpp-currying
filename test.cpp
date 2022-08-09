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

int main()
{
	auto cexpr = curry(expr);
	
	std::cout << "applying (1)\n";
	auto cexpr1 = cexpr(1);

	std::cout << "applying (2,4)\n";
	auto cexpr124 = cexpr1(2,4);

	std::cout << "applying (8)\n";
	auto cexpr1248 = cexpr124(8);

	std::cout << "printing result:\n";
	std::cout << cexpr1248 << "\n\n";

	std::cout << "printing curry(expr)(1,2,4,8):\n";
	std::cout << curry(expr)(1,2,4,8) << "\n\n";

	std::cout << "printing curry(expr)(1)(2)(4)(8):\n";
	std::cout << curry(expr)(1)(2)(4)(8) << "\n\n";

	std::cout << curry([](){std::cout << "curry(function with no arguments)() = "; return 100;})() << "\n\n";

	curry([](auto s){std::cout << s << "\n\n";})("curry(function with no return type)");

	std::cout << "curry(\"a raw value\") = " << '"' << curry("a raw value") << '"' << "\n\n";

	std::cout << "don't " << curry([](){
		std::cout << "forget "; return [](int a){
			std::cout << "your "; return [=](int b){
				std::cout << "unit "; return [=](){
					std::cout << "applications: "; return a + b;};};};})
		()(1,2)() << "\n\n";

	std::function<int(int,int)> f = curry(expr)(1)(2); //does *not* construct a new std::function

	char c; std::cin >> c;
}