# Sic functions

These are the built-in functions and macros defined by the `sic`
programming language.

This document was generated on Sat Jun 22 17:51:53 2019.

## `abs` (`abs_op` in C++)

`(abs arg1)`

Return the absolute value of the (numeric) argument.

## `add` (also `+`)

`(add arg1 arg2 ...)`

Addition. Accepts 2 or more arguments and returns the sum

## `and` (`and_op` in C++)

`(and expr1 expr2 ....)`

**Macro**

Short-circuited boolean 'and'.  Evaluates arguments until one is
nil; returns the result of the last argument evaluated.

The actual work is done by and-eval; this just quotes the
arguments.


## `and-eval` (`and_eval` in C++)

`(and-eval '(expr1 expr2 ...))`

Evaluate expressions left to right until one evaluates to false.
Returns the result of the last expression evaluated.

## `ceil` (`ceil_op` in C++)

`(ceil arg1)`

Round up

## `cond`

`(cond ( (cond-expr) (val-expr) ) ( (cond-expr-2)  ) ... )`

**Macro**

Classic Lisp conditional evaluation (more or less).

Takes a series of (usually) two-item lists of expressions and for
each list evaluates the first expression.  If the result is true,
it evaluates the second expression and returns its result.
Otherwise, it proceeds to the next list.

If all test expressions return `nil`, the result of the cond
expression is also nil.

If a condition list contains only one item and its first
(i.e. only) expression evaluates true, that is also the result of
the whole `cond` expression.

The heavy lifting for this is done by `cond-eval`.

## `cond-eval` (`cond_eval` in C++)

`(cond-eval '( ( (cond-expr) (val-expr) ) ( (cond-expr-2)  ) ... ) )`

Back-end for the `cond` and `or` macros.

Evaluates each cond-expr until one returns true, then evaluates and
returns the result of the corresponding val-expr.  If 'val-expr' is
omitted, returns the result of the cond-expr instead.

## `defmacro`

`(defmacro foo (a1 a2 a3) ... )`

**Macro**

Defines a macro and points a global at it.

Note that macro support in `sic` is still primitive.  In
particular, `sic` currently does not have a way to define variadic
functions in `sic` itself.  This means that most effective macros
are written in C++.

## `defun`

`(defun foo (a1 a2 a3) ... )`

**Macro**

Defines a function (i.e. a `fun`) and points a global at it.

## `div` (also `/`)

`(div arg1 arg2)`

Division

## `each` (`each_op` in C++)

`(each function list)`

Evaluate function over each item in the list, discarding the
result(s).

## `eq?` (`eq_p` in C++)

`(eq? arg1 arg2)`

Compare arguments for equality, as defined by the various types:

String          -- An identical sequence of characters
Number          -- Numerical equality
Pair and List   -- All elements are equal
All others:     -- Pointer equality (i.e. they're the same obj)

Objects must have the same type to be identical.

## `eval` (`eval_op` in C++)

`(eval '(+ 2 2))`

Evaluate the argument as an expression

## `first` (also `car`)

`(first list)`
Returns the first item in a list.

## `floor` (`floor_op` in C++)

`(floor arg1)`

Round down

## `fold`

`(fold fn initial list)`

Evaluate fn on each item in list, calling it two arguments: the
result of previous fn call and the current item.  For the first
item, the first argument for 'fn' is 'initial'.

## `fun`

`(fun (arg1 arg2 ...) body-statements)`

**Macro**

Defines and returns a `fun` (i.e. a function that will always
execute in the global scope) with the given arguments and body.

Most of the time, you will want to call `defun` or `lamba` instead.
The result is unnamed unless you explicitly assign it to a variable
and it does not have access to current local variables the way
lambda does.

Expands to a call to `make-function`.

## `ge` (also `>=`)

`(ge arg1 arg2)`

Tests if the first argument is greater than or equal to the second.
Arguments **must** be numbers.

## `gt` (also `>`)

`(gt arg1 arg2)`

Tests if the first argument is greater than the second.  Arguments
**must** be numbers.

## `if` (`if_op` in C++)

`(if (condition) (true-expr) (optional-false-expr) )`

**Macro**

Macro implementing the 'if' control structure.  Expands to a
`cond-eval` expression.

## `lambda`

`(lambda (arg1 arg2 ...) body-statements)`

**Macro**

Defines and returns a lambda (i.e. a function that captures the
local scope) with the given arguments and body.

Expands to a call to `make-function`.

## `le` (also `<=`)

`(le arg1 arg2)`

Tests if the first argument is less than or equal to the second.
Arguments *must* be numbers.

## `let`

`(let ((a value) b (c value) (d)) ... )`

**Macro**

Creates a local scope and defines zero or more local variables in,
then evaluates the remaining lists as expressions in this context.

Variable definitions may be either symbols or two-item lists
containing the symbol and an expression.  In the latter case, the
expression is evaluated and the result is stored in the variable.
Otherwise, the variable is initialized to nil.

One-item lists containing just the name are also allowed; they are
equivalent to the bare symbol.

## `let-eval` (`let_eval` in C++)

`(let-eval '((a value) b (c value) (d)) '(expr1 expr2 ...) )`

Evaluates a `let` operation.  First list defines the locals, the
second list is eval'd in their context.

Back-end function for the `let` macro.

## `list`

`(list ...)`

Return a list containing the values of each of the arguments in the
order they were given.  This is different from `'(a b c)` in that
the arguments will have been evaluated.

## `llen` (`llen_op` in C++)

`(llen arg1)`

Return the length of the given list or zero if the argument is not
a list.

## `lt` (also `<`)

`(lt arg1 arg2)`

Tests if the first argument is less than the second.  Arguments
*must* be numbers.

## `macro`

`(macro (arg1 arg2 ...) body-statements)`

**Macro**

Defines and returns a macro (i.e. a function that takes zero or
more unevaluated arguments and returns a new expression that is
then evaluated by the system (typically a modified form of the
given arguments, but that is not required.)

You will almost certainly want to use `defmacro` instead of this.

Expands to a call to `make-function`.

## `make-function` (`make_function` in C++)

`(make-function formals body lambda? macro?)`

If 'lambda?' is true, it evaluates in the defining function's scope
instead of the toplevel scope.

## `map` (`map_op` in C++)

`(map function list)`

Evaluate function over each item of the list and return a list of
the results.

## `mod` (`mod_op` in C++)

`(mod arg1 arg2)`

Modulo

## `mul` (also `*`)

`(mul arg1 arg2)`

Multiplication

## `ne?` (`ne_p` in C++)

`(ne? arg1 arg2)`

Compare items for inequalty.  True if and only if `eq?` would
return false on these arguments.

## `not` (`not_op` in C++)

`(not arg1)`

Boolean inversion.  Returns `nil` if the argument is true and `t`
if the argument is false.

## `nth`

`(nth a-list 4)`
Return the nth index of a list; zero-based.

## `or` (`or_op` in C++)

`(or expr1 expr2 ...)`

**Macro**


Short-circuited boolean `or` operation.  Evaluate arguments until
one is true, returning its value.  Remaining arguments are not
evaluated.  Returns nil if no true result is found.

Expands to a `cond` expression (e.g. `(cond-expr '( (expr1) (expr2) ...) )`)


## `pair` (`pair_op` in C++)

`(pair arg1 arg2)`

Create a pair object holding the two arguments.

## `print`

`(print ...)`

Print each argument's string representation to stdout.

## `progn`

`(progn ...)`

Return the value of the last argument, or nil if there is none.
This is used to evaluate a sequence of expressions; it's basically
'let' without local variables.

## `quote`

`(quote x)`

Returns x unevaluated.  This is treated as a special case by
`eval`, which evaluates calls to this function itself and prevents
the argument from being evaluated.  (Note: **NOT** functions with
this name; this specific function.  Assigning it to a new variable
will not defeat this, nor will creating a local function named
`quote`.)

If you somehow manage to trick `eval` into calling this function,
it will simply return its argument.

## `rest` (also `cdr`)

`(rest list)`
Returns a list without its first item.

## `round` (`round_op` in C++)

`(round arg1)`

Round toward nearest integral value

## `second` (also `cadr`)

`(second arg1)`

Returns the second item in a list or nil if there is none.

## `set`

`(set symbol value)`


## `setq`

`(setq sym value)`

**Macro**

Assignment macro.  Expands to `set` with `sym` quoted.

## `str-to-num` (`str_to_num` in C++)

`(str-to-num arg1)`

Given a string, attempt to parse it as a decimal number and return
the value as a Sic number.  Returns nil if this doesn't work.

## `sub` (also `-`)

`(sub arg1 arg2)`

Subtraction.

## `third` (also `caddr`)

`(third arg1)`

Returns the third item in a list or nil if there is none.

## `tl-set` (`tl_set` in C++)

`(tl-set global-symbol value)`

Like `set` but always only modifies the global namespace.  Creates
the variable if it doesn't exist.

## `trunc` (`trunc_op` in C++)

`(trunc arg1)`

Truncate toward zero

## `while` (`while_op` in C++)

`(while (condition) (expr1) ... )`

**Macro**

Basic `while` loop.

Evaluates the first expression and, if true, evaluates the
remaining expressions in order.  Repeats this until the first
expression evaluates to false.

