# C++ Currying

I whipped this up one morning after realizing I had never seen an implementation of *actual* currying in c++, let alone something ergonomic and powerful.

The function "curry" transforms a function, so that there is no difference between, for example, curry(f)(x,y,z), curry(f)(x)(y,z), curry(f)(x,y)(z), and curry(f)(x)(y)(z), regardless of if/how the original function was curried.

The result is retrieved via implicit conversion.

Functions with no arguments must be explicitly invoked with empty operator(), as if they take a unit type.

Intermediate function types of already partially-curried functions are not lost from being wrapped up into anonymous functions and can thus be retrieved via implicit conversion (so types overloaded with operator() won't just automatically get eaten up by curry).

I made everything constexpr, but did not worry about noexcept or the constness of operator().

There is, of course, no use of std::function or any form of allocation/overhead.
