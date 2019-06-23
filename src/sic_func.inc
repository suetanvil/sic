// This file is part of Sic; Copyright (C) 2019 The Author(s)
// LGPLv2 w/ exemption; NO WARRANTY! See Copyright.txt for details

// Files that #include this file must define at least BUILTIN_FULL:
//
// #define BUILTIN_FULL(name, min_args, is_varargs, is_macro)

// Convenience macro for the most common case (non-macro with fixed
// number of arguments)
#define BUILTIN(name, num_args) \
    BUILTIN ## _ ## FULL(name,num_args,true,false)

// Used to append stuff after the body.  Or not.
#ifndef ENDF
#   define ENDF
#endif

// #define ALIAS(op, new_name)
//
// used for defining alternate names for some functions.
// 
// new_name is a string
#ifndef ALIAS
#   define ALIAS(op, newname)
#endif



/// (lambda (arg1 arg2 ...) body-statements)
///
/// Defines and returns a lambda (i.e. a function that captures the
/// local scope) with the given arguments and body.
///
/// Expands to a call to `make-function`.
BUILTIN_FULL(lambda, 1, true, true)
#ifdef BODY
{
    return unnamed_fun_helper(args, true, false);
}
#endif
ENDF

/// (fun (arg1 arg2 ...) body-statements)
///
/// Defines and returns a `fun` (i.e. a function that will always
/// execute in the global scope) with the given arguments and body.
///
/// Most of the time, you will want to call `defun` or `lamba` instead.
/// The result is unnamed unless you explicitly assign it to a variable
/// and it does not have access to current local variables the way
/// lambda does.
///
/// Expands to a call to `make-function`.
BUILTIN_FULL(fun, 1, true, true)
#ifdef BODY
{
    return unnamed_fun_helper(args, false, false);
}
#endif
ENDF

/// (macro (arg1 arg2 ...) body-statements)
///
/// Defines and returns a macro (i.e. a function that takes zero or
/// more unevaluated arguments and returns a new expression that is
/// then evaluated by the system (typically a modified form of the
/// given arguments, but that is not required.)
///
/// You will almost certainly want to use `defmacro` instead of this.
///
/// Expands to a call to `make-function`.
BUILTIN_FULL(macro, 1, true, true)
#ifdef BODY
{
    return unnamed_fun_helper(args, false, true);
}
#endif
ENDF


/// (defun foo (a1 a2 a3) ... )
///
/// Defines a function (i.e. a `fun`) and points a global at it.
BUILTIN_FULL(defun, 3, true, true)
#ifdef BODY
{
    symbol *name = dca<symbol>(args[0]);
    args.erase(args.begin());

    obj *the_fun = unnamed_fun_helper(args, false, false);
    return $(tl_set, $(quote, name), the_fun);
}
#endif
ENDF


/// (defmacro foo (a1 a2 a3) ... )
///
/// Defines a macro and points a global at it.
///
/// Note that macro support in `sic` is still primitive.  In
/// particular, `sic` currently does not have a way to define variadic
/// functions in `sic` itself.  This means that most effective macros
/// are written in C++.
BUILTIN_FULL(defmacro, 3, true, true)
#ifdef BODY
{
    symbol *name = dca<symbol>(args[0]);
    args.erase(args.begin());

    obj *the_macro = unnamed_fun_helper(args, false, true);
    return $(tl_set, $(quote, name), the_macro);
}
#endif
ENDF



/// (while (condition) (expr1) ... )
///
/// Basic `while` loop.
///
/// Evaluates the first expression and, if true, evaluates the
/// remaining expressions in order.  Repeats this until the first
/// expression evaluates to false.
BUILTIN_FULL(while_op, 2, true, true)
#ifdef BODY
{
    obj *cond_expr = args[0];
    args.erase(args.begin());

    while(eval(cond_expr, ctx)->isTrue()) {
        for (obj* expr : args) {
            eval(expr, ctx);
        }
    }

    return nil;
}
#endif
ENDF


/// (cond-eval '( ( (cond-expr) (val-expr) ) ( (cond-expr-2)  ) ... ) )
///
/// Back-end for the `cond` and `or` macros.
///
/// Evaluates each cond-expr until one returns true, then evaluates and
/// returns the result of the corresponding val-expr.  If 'val-expr' is
/// omitted, returns the result of the cond-expr instead.
BUILTIN(cond_eval, 0)
#ifdef BODY
{
    for (obj *curr = dca<pair>(args[0]);
         curr->isTrue();
         curr = basic_rest(curr))
    {
        obj *curr_expr = basic_first(curr);

        obj *cond_result = eval( basic_first(curr_expr), ctx);
        if (cond_result != nil) {
            if (llen(curr_expr) <= 1) { return cond_result; }
            return eval( basic_second(curr_expr), ctx );
        }
    }

    return nil;
}
#endif
ENDF


/// (cond ( (cond-expr) (val-expr) ) ( (cond-expr-2)  ) ... )
///
/// Classic Lisp conditional evaluation (more or less).
///
/// Takes a series of (usually) two-item lists of expressions and for
/// each list evaluates the first expression.  If the result is true,
/// it evaluates the second expression and returns its result.
/// Otherwise, it proceeds to the next list.
///
/// If all test expressions return `nil`, the result of the cond
/// expression is also nil.
///
/// If a condition list contains only one item and its first
/// (i.e. only) expression evaluates true, that is also the result of
/// the whole `cond` expression.
///
/// The heavy lifting for this is done by `cond-eval`.
BUILTIN_FULL(cond, 0, true, true)
#ifdef BODY
{
    obj *clauses = vec2list(args);

    return $(cond_eval, $(quote, clauses));
}
#endif
ENDF

/// (or expr1 expr2 ...)
///

///
/// Short-circuited boolean `or` operation.  Evaluate arguments until
/// one is true, returning its value.  Remaining arguments are not
/// evaluated.  Returns nil if no true result is found.
///
/// Expands to a `cond` expression (e.g. `(cond-expr '( (expr1) (expr2) ...) )`)
///
BUILTIN_FULL(or_op, 0, true, true)
#ifdef BODY
{
    std::vector<obj*> expressions;
    for (obj* expr : args) {
        expressions.push_back( $( expr ) );
    }

    obj *clauses = vec2list(expressions);
    return $(cond_eval, $(quote, clauses));
}
#endif
ENDF


/// (and-eval '(expr1 expr2 ...))
///
/// Evaluate expressions left to right until one evaluates to false.
/// Returns the result of the last expression evaluated.
BUILTIN(and_eval, 1)
#ifdef BODY
{
    obj *result = nil;

    for (obj* curr = args[0]; curr != nil; curr = basic_rest(curr)) {
        obj* expr = basic_first(curr);
        result = eval(expr, ctx);
        if (result == nil) { break; }
    }

    return result;
}
#endif
ENDF


/// (and expr1 expr2 ....)
///
/// Short-circuited boolean 'and'.  Evaluates arguments until one is
/// nil; returns the result of the last argument evaluated.
///
/// The actual work is done by and-eval; this just quotes the
/// arguments.
///

BUILTIN_FULL(and_op, 0, true, true)
#ifdef BODY
{
    return $(and_eval, $(quote, vec2list(args)));
}
#endif
ENDF



/// (eval '(+ 2 2))
///
/// Evaluate the argument as an expression
BUILTIN(eval_op, 1)
#ifdef BODY
{
    return eval(args[0], ctx);
}
#endif
ENDF


/// (if (condition) (true-expr) (optional-false-expr) )
///
/// Macro implementing the 'if' control structure.  Expands to a
/// `cond-eval` expression.
BUILTIN_FULL(if_op, 2, true, true)
#ifdef BODY
{
    obj *test_expr = args[0];
    obj *true_expr = args[1];

    if (args.size() > 3) {
        throw bad_arg("2 or 3 arguments", "more");
    }

    obj *expr = args.size () == 3
        ?
        $(cond_eval,
          $(quote,
            $( $(test_expr, true_expr),
               $(t,    args[2]) )))
        :
        $(cond_eval,
          $(quote,
            $( $(test_expr, true_expr) )))
        ;

    return expr;
}
#endif
ENDF


/// (make-function formals body lambda? macro?)
///
/// If 'lambda?' is true, it evaluates in the defining function's scope
/// instead of the toplevel scope.
BUILTIN(make_function, 4)
#ifdef BODY
{
    pair *formals = dca<pair>(args[0]);
    pair *body = dca<pair>(args[1]);
    bool isLambda = args[2] != nil;
    bool isMacro = args[3] != nil;

    return new function(formals, body,
                        isLambda ? ctx : ctx->root(), isMacro);
}
#endif
ENDF


/// (set symbol value)
BUILTIN(set, 2)
#ifdef BODY
{
    symbol *sym = dynamic_cast<symbol*>(args[0]);
    obj *val = args[1];

    // We do the sanity check here because typing 'set' instead of
    // 'setq' is an easy mistake that's otherwise hard to track
    // down (not that I would know or anything).
    if (!sym) {
        throw wrong_type("'set' first argument is not a symbol!");
    }

    ctx->tl_set(sym->text, val);
    return args[1];
}
#endif
ENDF

/// (setq sym value)
///
/// Assignment macro.  Expands to `set` with `sym` quoted.
BUILTIN_FULL(setq, 2, false, true)
#ifdef BODY
{
    return $(set, $(quote, args[0]), args[1]);
}
#endif
ENDF


/// (tl-set global-symbol value)
///
/// Like `set` but always only modifies the global namespace.  Creates
/// the variable if it doesn't exist.
BUILTIN(tl_set, 2)
#ifdef BODY
{
    ctx->root()->tl_set(dca<symbol>(args[0])->text, args[1]);
    return args[1];
}
#endif
ENDF


/// (let-eval '((a value) b (c value) (d)) '(expr1 expr2 ...) )
///
/// Evaluates a `let` operation.  First list defines the locals, the
/// second list is eval'd in their context.
///
/// Back-end function for the `let` macro.
BUILTIN(let_eval, 2)
#ifdef BODY
{
    context *newctx = new context(ctx);

    pair *locals = dca<pair>(args[0]);
    pair *body = dca<pair>(args[1]);

    basic_each(
        locals,
        [newctx](obj *local) {
            if (local->isSymbol()) {
                newctx->define(dca<symbol>(local)->text, nil);
            } else if (local->isList()) {
                pair *lp = dca<pair>(local);
                symbol *name = dca<symbol>(lp->first);
                obj *value = eval(dca<pair>(lp->rest)->first, newctx);

                newctx->define(name->text, value);
            } else {
                throw wrong_type("symbol or list", printstr(local));
            }
        });

    return eval(body, newctx);
}
#endif
ENDF


/// (let ((a value) b (c value) (d)) ... )
///
/// Creates a local scope and defines zero or more local variables in,
/// then evaluates the remaining lists as expressions in this context.
///
/// Variable definitions may be either symbols or two-item lists
/// containing the symbol and an expression.  In the latter case, the
/// expression is evaluated and the result is stored in the variable.
/// Otherwise, the variable is initialized to nil.
///
/// One-item lists containing just the name are also allowed; they are
/// equivalent to the bare symbol.
BUILTIN_FULL(let, 1, true, true)
#ifdef BODY
{
    pair *locals = dca<pair>(args[0]);

    args.erase(args.begin());
    obj *body = new pair(progn, vec2list(args));

    return $(let_eval, $(quote, locals), $(quote, body));
}
#endif
ENDF


/// (quote x)
///
/// Returns x unevaluated.  This is treated as a special case by
/// `eval`, which evaluates calls to this function itself and prevents
/// the argument from being evaluated.  (Note: **NOT** functions with
/// this name; this specific function.  Assigning it to a new variable
/// will not defeat this, nor will creating a local function named
/// `quote`.)
///
/// If you somehow manage to trick `eval` into calling this function,
/// it will simply return its argument.
BUILTIN(quote, 1)
#ifdef BODY
{
    return args[0];
}
#endif
ENDF

/// Addition. Accepts 2 or more arguments and returns the sum
ALIAS(add, "+")
BUILTIN_FULL(add, 2, true, false)
#ifdef BODY
{
    double sum = 0;
    for(obj* i : args) {
        sum += dca<number>(i)->val;
    }

    return new number(sum);
}
#endif
ENDF

/// Subtraction.
ALIAS(sub, "-")
BUILTIN(sub, 2)
#ifdef BODY
{
    return new number(dca<number>(args[0])->val - dca<number>(args[1])->val);
}
#endif
ENDF

/// Multiplication
ALIAS(mul, "*")
BUILTIN(mul, 2)
#ifdef BODY
{
    return new number(dca<number>(args[0])->val * dca<number>(args[1])->val);
}
#endif
ENDF

/// Division
ALIAS(div, "/")
BUILTIN(div, 2)
#ifdef BODY
{
    return new number(dca<number>(args[0])->val / dca<number>(args[1])->val);
}
#endif
ENDF

/// Modulo
ALIAS(mod_op, "%")
BUILTIN(mod_op, 2)
#ifdef BODY
{
    long n = (long)trunc( dca<number>(args[0])->val );
    long d = (long)trunc( dca<number>(args[1])->val);
    return new number(n % d);
}
#endif
ENDF

/// Truncate toward zero
BUILTIN(trunc_op, 1)
#ifdef BODY
{
    return new number(trunc( dca<number>(args[0])->val ));
}
#endif
ENDF

/// Round down
BUILTIN(floor_op, 1)
#ifdef BODY
{
    return new number(floor( dca<number>(args[0])->val ));
}
#endif
ENDF

/// Round up
BUILTIN(ceil_op, 1)
#ifdef BODY
{
    return new number(ceil( dca<number>(args[0])->val ));
}
#endif
ENDF

/// Round toward nearest integral value
BUILTIN(round_op, 1)
#ifdef BODY
{
    return new number(round( dca<number>(args[0])->val ));
}
#endif
ENDF

/// Print each argument's string representation to stdout.
BUILTIN_FULL(print, 0, true, false)
#ifdef BODY
{
    for(obj* i : args) {
        std::cout << printstr(i, ctx, false);
    }

    return nil;
}
#endif
ENDF

/// Return the value of the last argument, or nil if there is none.
/// This is used to evaluate a sequence of expressions; it's basically
/// 'let' without local variables.
BUILTIN_FULL(progn, 0, true, false)
#ifdef BODY
{
    return (args.size() == 0) ? nil : args.back();
}
#endif
ENDF

/// Compare arguments for equality, as defined by the various types:
///
///  String          -- An identical sequence of characters
///  Number          -- Numerical equality
///  Pair and List   -- All elements are equal
///  All others:     -- Pointer equality (i.e. they're the same obj)
///
/// Objects must have the same type to be identical.
ALIAS(eq_p, "==")
BUILTIN(eq_p, 2)
#ifdef BODY
{
    if (args[0]->equals(args[1])) { return t; }
    return nil;
}
#endif
ENDF

/// Compare items for inequalty.  True if and only if `eq?` would
/// return false on these arguments.
ALIAS(ne_p, "!=")
BUILTIN(ne_p, 2)
#ifdef BODY
{
    if (args[0]->equals(args[1])) { return nil; }
    return t;
}
#endif
ENDF

/// Boolean inversion.  Returns `nil` if the argument is true and `t`
/// if the argument is false.
ALIAS(not_op, "!")
BUILTIN(not_op, 1)
#ifdef BODY
{
    if (args[0] == nil) { return t; }
    return nil;
}
#endif
ENDF

/// Return a list containing the values of each of the arguments in the
/// order they were given.  This is different from `'(a b c)` in that
/// the arguments will have been evaluated.
BUILTIN_FULL(list, 0, true, false)
#ifdef BODY
{
    return vec2list(args);
}
#endif
ENDF

/// Tests if the first argument is less than the second.  Arguments
/// *must* be numbers.
ALIAS(lt, "<")
BUILTIN(lt, 2)
#ifdef BODY
{
    return dca<number>(args[0])->val < dca<number>(args[1])->val
        ? (obj*)t : (obj*)nil;
}
#endif
ENDF

/// Tests if the first argument is less than or equal to the second.
/// Arguments *must* be numbers.
ALIAS(le, "<=")
BUILTIN(le, 2)
#ifdef BODY
{
    return dca<number>(args[0])->val <= dca<number>(args[1])->val
        ? (obj*)t : (obj*)nil;
}
#endif
ENDF

/// Tests if the first argument is greater than the second.  Arguments
/// **must** be numbers.
ALIAS(gt, ">")
BUILTIN(gt, 2)
#ifdef BODY
{
    return dca<number>(args[0])->val > dca<number>(args[1])->val
        ? (obj*)t : (obj*)nil;
}
#endif
ENDF

/// Tests if the first argument is greater than or equal to the second.
/// Arguments **must** be numbers.
ALIAS(ge, ">=")
BUILTIN(ge, 2)
#ifdef BODY
{
    return dca<number>(args[0])->val >= dca<number>(args[1])->val
        ? (obj*)t : (obj*)nil;
}
#endif
ENDF

/// Given a string, attempt to parse it as a decimal number and return
/// the value as a Sic number.  Returns nil if this doesn't work.
BUILTIN(str_to_num, 1)
#ifdef BODY
{
    try {
        double d = std::stod( dca<string>(args[0])->contents );
        return new number(d);
    } catch(std::invalid_argument) {
        return nil;
    } catch(std::out_of_range) {
        return nil;
    }
}
#endif
ENDF

/// (first list)
/// Returns the first item in a list.
ALIAS(first, "car")
BUILTIN(first, 1)
#ifdef BODY
{
    return basic_first(args[0]);
}
#endif
ENDF

/// (rest list)
/// Returns a list without its first item.
ALIAS(rest, "cdr")
BUILTIN(rest, 1)
#ifdef BODY
{
    return basic_rest(args[0]);
}
#endif
ENDF

/// Returns the second item in a list or nil if there is none.
ALIAS(second, "cadr")
BUILTIN(second, 1)
#ifdef BODY
{
    return basic_first(basic_rest(args[0]));
}
#endif
ENDF

/// Returns the third item in a list or nil if there is none.
ALIAS(third, "caddr")
BUILTIN(third, 1)
#ifdef BODY
{
    return basic_first(basic_rest(basic_rest(args[0])));
}
#endif
ENDF

/// (nth a-list 4)
/// Return the nth index of a list; zero-based.
BUILTIN(nth, 2)
#ifdef BODY
{
    return basic_nth(args[0], (int)trunc(dca<number>(args[1])->val));
}
#endif
ENDF

/// Create a pair object holding the two arguments.
ALIAS(pair_op, "cons")
BUILTIN(pair_op, 2)
#ifdef BODY
{
    return new pair(args[0], args[1]);
}
#endif
ENDF

/// Return the length of the given list or zero if the argument is not
/// a list.
BUILTIN(llen_op, 1)
#ifdef BODY
{
    pair *list = dynamic_cast<pair*>(args[0]);
    return new number( list ? (long)llen(list) : 0L );
}
#endif
ENDF

/// Return the absolute value of the (numeric) argument.
BUILTIN(abs_op, 1)
#ifdef BODY
{
    return new number( fabs( dca<number>(args[0])->val ) );
}
#endif
ENDF


/// (map function list)
///
/// Evaluate function over each item of the list and return a list of
/// the results.
BUILTIN(map_op, 2)
#ifdef BODY
{
    callable *func  = dca<callable>(args[0]);
    pair *list      = dca<pair>(args[1]);

    return basic_map(
        list,
        [=](obj* item) { return func->call($(item), ctx); }
        );
    
}
#endif
ENDF


/// (each function list)
///
/// Evaluate function over each item in the list, discarding the
/// result(s).
BUILTIN(each_op, 2)
#ifdef BODY
{
    callable *func  = dca<callable>(args[0]);
    pair *list      = dca<pair>(args[1]);

    basic_each(
        list,
        [=](obj* item) { return func->call($(item), ctx); }
        );
    
    return nil;
}
#endif
ENDF


/// (fold fn initial list)
///
/// Evaluate fn on each item in list, calling it two arguments: the
/// result of previous fn call and the current item.  For the first
/// item, the first argument for 'fn' is 'initial'.
BUILTIN(fold, 3)
#ifdef BODY
{
    callable *func  = dca<callable>(args[0]);
    obj *initial    = args[1];
    pair *list      = dca<pair>(args[2]);

    obj *result = initial;
    basic_each(
        list,
        [&](obj* item) {
            result = func->call($(result, item), ctx); }
        );
    
    return result;
}
#endif
ENDF



#undef BUILTIN
#undef ENDF
#undef BUILTIN_FULL
#undef BODY
#undef ALIAS
