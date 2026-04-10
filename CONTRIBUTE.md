# How to contribute

If you're reading this: thank you for considering to contribute. It means a lot
to us!

## General

The very fist thing you need to do is read the
[Contribution Policy Agreement](CONTRIBUTION-POLICY-AGREEMENT.md) very
carefully. Once you did and keep all these guidelines in mind, you're good to
go! 

There are some more specific considerations for contributing to the software
for misfits, which will be outlined below.

## Software

### Coding Style

Generally speaking, if you know the Linux Kernel Coding Style, you're already
good. If not, here's a TL;DR

1. 8-Space-long tabs for indentation. 
2. Try to keep an 80-column limit per line. If it's a little bit over, and 
   splitting it into multiple lines makes it harder to read, it's okay to break
   this rule.
3. K&R Coding style. Writing C like it's C# is just *yuck*
4. No more than 3 indentations deep (when it comes to logic, it doesn't matter
   that much with structs)
5. Use a space after most keywords (if, switch, case, for, do, while)
6. Global variables and functions (which should only be used if you *really* 
   need them) need have descriptive names. A function that counts the active
   amount of users should be called "count_active_users()" or similar.
7. Do not encode the datatype of the function in to the name.
8. Local Variable names should be short and to the point, e.g. a return value
   should be called "ret", not "return_value". Similar, an index for
   a for-loop should be called "i", not "loop_counter".
9. Be careful with typedefs. There should always be a good reason for using it.
   See [Linux Kernel Coding Style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html#typedefs)
   for acceptable uses of typedefs.
10. Functions should be short and do just one thing. They should fit on one or
    two screenfuls of text (no, your vertical ultrawide doesn't count). It should
    do one thing and one thing well.
11. If you have a fairly simple function that just has one long case-statement,
    that's of course fine, it still does just one thing and is *logically* short.
12. If your function has more than 5-10 local variables, you might need to rethink
    if you should split that into multiple functions.
13. You can centralise exiting your functions using goto statements. Refer to
    the [Linux Kernel Coding Style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html#centralized-exiting-of-functions)
    once again for examples.
14. Don't over-comment.
15. In comments, explain *what* your code does, not *how*. Avoid comment blocks
    inside functions, if you need to do this, you might want to revisit point 10.
16. Be careful with macros and conditional compilation.
17. Macros should be named in all-caps (e.g. FOO(x) instead of foo(x))
18. If possible, avoid conditional compilations using macros. They are the devil.

