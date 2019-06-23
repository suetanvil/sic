# Sic: Yet Another Mediocre Lisp Dialect in C++

The other day, I had an interesting realization about modern C++.  One
thing led to another and here I am with another Lisp dialect.  Sorry
about that.

## Why?

It seemed like a good idea at the time.


## What's it good for?

`¯\_(ツ)_/¯`

Also,

### 1. It's simple.

Most Lispish languages care about fripperies like efficiency and so
will internally convert Lisp(ish) expressions to more efficient
forms.

Not Sic.  Here, it's still a list.  If you prod at one with `lldb`,
you can chase the pointers.  (Don't though; call function `po` on it
instead.)  It stays Lispy all the way down.


### 2. It integrates nicely with C++

Everything in Sic is a plain old C++ type, so you can easily move
between it and native code.  In fact, this:

    $(print, "2 + 2 =", $(add, 2, 2), "\n")

is a valid C++ expression that does what you'd expect (if you've
imported the `sic` namespace).  So it's easy to integrate it into a
C++ program.

### 3. No external files needed

Unlike most Lispish language, there is no external library; all
functions are written in C++.

### 4. You can horrify Lisp purists with it.

    $(print, "C++ is an acceptable Lisp!\n")


## What's wrong with it?

1. There's a lot of powerful Lisp(ish) functionality that isn't
   written yet.
2. There's no garbage collection.  It just leaks memory.


## Compiling

On a sufficiently Unix-like operating system with Clang installed, just

    cd src
    make

If you want to use gcc instead, edit the Makefile first.

Note that this needs a recent compiler, one that supports C++17.

This will produce both an executable interpreter (`sic`) and a library
(`libsic.a`).

## Documentation

The reference manual is generated during building but there's a
courtesy copy [here](doc/function-reference.md).

I also did a blog post about it
[here](http://www.blit.ca/blog/fdey966ldd/Sic__Yet_Another_Mediocre_Small_Lisp_Dialect.html).


## License

This is Free/Open-source software released under the terms of the
wxWidgets license (i.e. the GNU LGPL but with less restrictions on
binary distribution.)

See [Copyright.txt](Copyright.txt) for details.
