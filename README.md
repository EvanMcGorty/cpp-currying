# C++ Currying

I whipped this up one morning after realizing I had never seen an implementation of *actual* currying in c++, let alone something ergonomic and powerful.

The generic class "curry" transforms a function, so that there is no difference between, for example, curry{f}(x,y,z), curry{f}(x)(y,z), curry{f}(x,y)(z), and curry{f}(x)(y)(z), regardless of if/how the original function was curried.

The result of an application can then be retrieved via implicit conversion, or alternatively via the public field curry::curry_wrapped_val (which is of type curry::curry_wrapped_val_type, the same type as the template parameter).

Functions with no arguments must be explicitly invoked with empty operator(), as if they take a unit type.

Intermediate function types of already partially-curried functions are not lost from being wrapped up into anonymous functions and can thus be retrieved via implicit conversion (so types with overloads of operator() won't just automatically get eaten up by curry).

The semantics are nearly identical to those of std::bind_front, so the user is expected to use std::ref/cref as necessary. The only exception is that curry::operator() called from a (lvalue) reference will wrap the callable object in a std::ref, which prevents it from later being able to have its rvalue operator() called, so explicitly copying and/or std::move-ing it is often desireable.

There is also a concept, "curried", which can represent instances of the class "curry" that can take particular parameters, where void can be used to indicate a unit argument aka empty application. When all arguments are left out, it just represents any instance of "curry".

I made everything constexpr and qualifier sensetive but did not worry about noexcept.

There is, of course, no use of std::function or any form of allocation or overhead.
