// This file is part of Sic; Copyright (C) 2019 The Author(s)
// LGPLv2 w/ exemption; NO WARRANTY! See Copyright.txt for details

#include <string>
#include <exception>
#include <map>
#include <vector>
#include <istream>
#include <iostream>
#include <cstring>

#include "sic.hpp"

namespace sic {


static obj *basic_read(std::istream& in);


pair * const nil = nilClass::getInstance();
pair * const null = nil;     // 'cuz nil is reserved by lldb
symbol * const t = dca<symbol>($$("t"));




// Back-end helper function for lambda and function: Turn 'lambda',
// 'function' and 'macro' expressions into 'make-function' calls.  The
// only difference is the value of the third argument.
static obj *unnamed_fun_helper(std::vector<obj*> args,
                               bool isLambda,
                               bool isMacro) {
    pair *formals = dca<pair>(args[0]);
    args.erase(args.begin());

    pair *body = nil;
    for (auto i = args.rbegin(); i != args.rend(); ++i) {
        body = new pair(*i, body);
    }

    return $(make_function, $(quote, formals), $(quote, body),
             isLambda ? (obj*)t : (obj*)nil,
             isMacro  ? (obj*)t : (obj*)nil);
}

//
// Define the builtins.
//
// We define them all as functions (with the usual name but with _CFN
// appended) to make it easy to set breakpoints on them.
//

#define BODY
#define BUILTIN_FULL(name, min_args, is_varargs, is_macro)    \
    callable * const name = \
        new builtin(min_args, is_varargs, is_macro,     \
                    [](std::vector<obj*> args, context* ctx) -> obj*
#define ENDF );

#include "sic_func.inc"



std::size_t
llen(obj *lst_obj) {
    std::size_t len = 0;

    pair *lst = dynamic_cast<pair*>(lst_obj);
    if (!lst) { return 0; }

    for (pair *cel = lst; cel != nil; cel = dca<pair>(cel->rest)) {
        ++len;
        if (cel->rest->isAtom()) { throw wrong_type("list", printstr(cel->rest));}
    }

    return len;
}// llen


static obj *
reverse_helper(obj *list, obj *reversed) {
    if (list == nil) { return reversed; }

    pair *plist = dca<pair>(list);
    return reverse_helper(plist->rest, new pair(plist->first, reversed));
}

pair *
reverse(pair *list) {
    return dca<pair>(reverse_helper(list, nil));
}// reverse


// Evaluate one expression.
obj*
eval(obj* expr, context* ctx) {
    try {
        if (expr->isSymbol()) { return ctx->get(dca<symbol>(expr)->text); }
        if (expr == nil || expr->isAtom()) { return expr; }

        if (!expr->isList())  { throw malformed_expr(); }


        pair *pexpr = dca<pair>(expr);
        pair *expr_args = dca<pair>(pexpr->rest);

        // 'quote' is a special case
        if (pexpr->first == quote) {
            if (llen(expr_args) != 1) { throw fn_arg_mismatch(); }
            return expr_args->first;
        }

        // Retrieve the function object; this entails eval'ing the
        // first item in expr.
        obj *fun = eval(pexpr->first, ctx);
        if (!fun->isCallable()) { throw not_a_function(); }

        // If this is a macro, expand it and eval() the result
        if (dca<callable>(fun)->isMacro) {
            obj* new_expr = dca<callable>(fun)->call(expr_args, ctx);
            return eval(new_expr, ctx);
        }// if

        // Evaluate the arguments and create a new list containing the
        // results.
        obj *actual = basic_map(
            expr_args,
            [&](obj* item) -> obj* {
                return eval(item, ctx);
            });

        return dca<callable>(fun)->call(actual, ctx);
    } catch (error &e) {
        e.addtrace(printstr(expr, ctx));
        throw;
    }
}// eval


obj*
function::call(obj* actualArgs, context*) const {
    context* ctx = new context(outer);

    pair *args = dca<pair>(actualArgs);
    assert(args);

    // Bind the argument values to their corresponding variables
    if (llen(formals) != llen(args)) { throw fn_arg_mismatch(); }
    obj *curr_arg = args;
    basic_each(
        formals,
        [&](obj* curr) {
            ctx->define(dca<symbol>(curr)->text,
                        dca<pair>(curr_arg)->first);
            curr_arg = dca<pair>(curr_arg)->rest;
        });

    // Evaluate the function body.
    obj *result = nil;
    basic_each(
        body,
        [&](obj* subexpr) {
            result = eval(subexpr, ctx);
        });

    return result;
}// call


obj*
builtin::call(obj* actualArgs, context* outer) const {
    std::size_t naa = llen(dca<pair>(actualArgs));
    if ( (!isVariadic && naa != nargs) || naa < nargs) {
        throw arg_count(nargs, naa);
    }

    std::vector<obj*> args;
    args.reserve(naa);

    for (obj *c = actualArgs; c != nil; c = dca<pair>(c)->rest) {
        args.push_back(dca<pair>(c)->first);
    }

    try {
        return code(args, outer);
    } catch (error& err) {
        obj *argfix = new pair((obj*)this, actualArgs);
        err.addtrace(printstr(argfix, outer));
        throw;
    }
}// builtin::call


std::string printstr(obj *o, const context *ctx, bool forDebugging) {
    // null is invalid but handling it here makes debugging easier
    if (!o) { return "{nullptr}"; }

    if (o == quote) { return "quote"; }

    if (o->isString()) {
        std::string result = o->str();
        if (forDebugging) { result = "\"" + result + "\""; }
        return result;
    }

    if (o == nil) { return o->str(); }

    // If we have a context handy, try to lookup the name of the
    // current function.
    if (ctx && o->isCallable()) {
        std::string name = ctx->name_of(o);
        if (name.size() == 0) { name = "unnamed callable"; }
        return "[" + name + "]";
    }// if 

    if (o->isList()) {
        std::string result = "(";

        bool first = true;
        for(obj *c = o; c != nil; c = dca<pair>(c)->rest) {
            if (!first) { result += ' '; }
            result += printstr(dca<pair>(c)->first, ctx);
            first = false;
        }
        result += ")";

        return result;
    }


    return o->str();
}// printstr


void
basic_each(obj *list, std::function<void(obj *)> actor) {
    for (obj *curr = dca<pair>(list);
         curr != nil;
         curr = dca<pair>(curr)->rest)
    {
        actor(dca<pair>(curr)->first);
    }// for
}// basic_each

// Indexed from zero, 'cuz we live in C++ land
obj *
basic_nth(obj *list, int index) {
    if (index < 0) { return nil; }

    if (list->isAtom()) { return nil; }

    for (pair *curr = dca<pair>(list);
         curr != nil;
         curr = dca<pair>(curr->rest))
    {
        if (index == 0) { return curr->first; }
        --index;

        if (curr->rest->isAtom()) { return nil; }
    }// for

    return nil;
}// basic_nth


// Convert a C++ std::vector<obj*> object to a list.
pair *
vec2list(const std::vector<obj*>& vec) {
    pair *result = nil;
    pair *last = nil;

    for (obj *item : vec) {
        pair* next = new pair(item, nil);

        if (result == nil) {
            result = next;
        } else {
            // We directly write next to the 'rest' of the previous
            // item.  This violates the constness of pair, which is
            // bad, but it improves simplicity and performance, which
            // is good.
            *(const_cast<obj**>(&(last->rest))) = next;
        }
        last = next;
    }// for

    return result;
}// vec2list


// Apply 'actor' to each item in 'list', returning a new list
// containing the result of calling 'actor'.
pair *
basic_map(pair *list, std::function<obj*(obj *)> actor) {
    std::vector<obj*> result;

    for (pair *curr = list; curr != nil; curr = dca<pair>(curr->rest)) {
        result.push_back(actor(curr->first));
    }// for

    return vec2list(result);
}// map


// Convert 'nm' from C++ naming convention to sic naming convention:
//
// 1. Trailing "_op" is dropped.
// 2. Trailing "_p" is replaced with "?"
// 3. All underscored ("_") are replaced with dashes ("-")
// 
static std::string fixname(const std::string& nm) {
    std::string name;

    std::string suffix = nm.size() >= 3 ? nm.substr(nm.size() - 3, 3) : "...";
    
    if (suffix == "_op") {
        name = nm.substr(0, nm.size() - 3);
    } else if (suffix.substr(1, 2) == "_p") {
        name = nm.substr(0, nm.size() - 2) + "?";
    } else {
        name = nm;
    }// if

    for (char &c : name) {
        if (c == '_') { c = '-'; }
    }// for 
    
    return name;
}// fixname


// Return a context suitable for use as the root of execution; that
// is, one with no parent that has been initialized with all of the
// functions, macros and other global constants.
context *
root_context() {
    context *tl = new context(nullptr);

    // Bindings for all built-in functions and their aliases
#define BUILTIN_FULL(name, x1,x2,x3)    tl->define(fixname(#name), name);
#define ALIAS(name, alias)              tl->define(alias, name);
#include "sic_func.inc"

    // Constants
    tl->define("nil", nil);
    tl->define("null", null);
    tl->define("t", t);

    // This may be set in repl.cpp; it's defined here for consistency
    tl->define("argv", nil);

    return tl;
}





//
// Read function
//

static void
skip_spaces(std::istream& in) {
    while (true) {
        // Advance to the next non-whitespace character
        while(in.good() && isspace(in.peek())) { in.get(); }

        // If this is a comment, it it and continue; comments are also
        // considered whitespace.
        char c = in.peek();
        if (c == ';' || c == '#') {
            std::string line;
            std::getline(in, line);
        } else {
            break;
        }
    }// while
}// skip_spaces



static obj *
read_list(std::istream& in) {
    std::vector<obj*> result;  // 'cuz who wants to reverse a list?

    char pk = in.get();
    assert(pk == '(');

    while(true) {
        skip_spaces(in);

        if (in.peek() == ')') {
            in.get();
            break;
        }

        if (in.eof()) {
            throw syntax_error("Unterminated list.");
        }

        result.push_back(basic_read(in));
    }// while

    return vec2list(result);
}// read_list

static obj *
read_string(std::istream& in) {
    std::string s = "";

    assert(in.get() == '"');

    while(true) {
        if (in.eof()) { throw syntax_error("Unterminated string!"); }

        char c = in.get();
        if (c == '"') { break; }
        if (c == '\\') {
            c = in.get();
            switch(c) {
            case 'a':   s += "\a"; break;
            case 'b':   s += "\b"; break;
            case 'f':   s += "\f"; break;
            case 'n':   s += "\n"; break;
            case 't':   s += "\t"; break;
            case 'v':   s += "\v"; break;
            default:    s += c;
            }
            continue;
        }// if

        s += c;
    }// while

    return new string(s);
}// read_string

static obj *
read_number(std::istream& in) {
    double n = 0;

    bool negate = false;
    if (in.peek() == '-') {
        negate = true;
        in.get();
    }

    char curr;
    while(true) {
        curr = in.get();
        if (in.eof() || !isdigit(curr)) { break; }

        n *= 10.0;
        n += (double)(curr - '0');
    }

    if (in.eof() || curr != '.') {
        if (!in.eof()) { in.putback(curr); }
        return new number(negate ? -n : n);
    }

    // Handle the fractional part if present
    double pos = 10.0;
    while(true) {
        curr = in.get();
        if (in.eof()) { break; }

        if (!isdigit(curr)) {
            in.putback(curr);
            break;
        }

        n += (double) (curr - '0') / pos;
        pos *= 10.0;
    }// while

    return new number(negate ? -n : n);
}// read_number

static obj *
read_word(std::istream& in, const std::function<bool(char c)>& validp) {
    std::string result = "";

    for (char c = in.peek(); !in.eof() && validp(c); c = in.peek()) {
        result += in.get();
    }// for

    return $$(result);
}// read_word

// Special case to distinguish between '-42' (a number) and '-+--' (an
// operator).
static obj *
read_word_or_number(std::istream& in, const std::function<bool(char c)>& validp) {
    char minus = in.get();
    assert(minus == '-');

    bool digit = isdigit(in.peek());
    in.putback(minus);

    if (digit) {
        return read_number(in);
    } else {
        return read_word(in, validp);
    }
}// read_word_or_number


// Return null on eof
static obj *
basic_read(std::istream& in) {
    std::vector<obj*> result;  // 'cuz who wants to reverse a list?

    // Word-style names must begin with a letter and contain only
    // letters, numbers or one of the characters in nonAlphaWords.
    // (The letter-first constraint is enforced below in the 'if'
    // statement.)
    const static std::string nonAlphaWords = "-_?!";
    auto isWord =
        [](char c) -> bool {
            return isalnum(c) || std::string::npos != nonAlphaWords.find(c);
        };

    // Operator-style names consist of one or more of the characters
    // in 'ops'.
    const static std::string ops = "-+=~!@$%^&*|\\//?<>~";
    auto isOperator =
        [](char c) { return std::string::npos != ops.find(c); };

    skip_spaces(in);
    if (in.eof()) { return nullptr; }

    char pk = in.peek();

    if (pk == '(')     { return read_list(in); }
    if (pk == '\'')    {
        in.get();
        return $(quote, basic_read(in));
    }
    if (pk == '"')     { return read_string(in); }
    if (pk == '-')     { return read_word_or_number(in, isOperator); }
    if (isdigit(pk))   { return read_number(in); }
    if (isalpha(pk))   { return read_word(in, isWord); }
    if (ispunct(pk))   { return read_word(in, isOperator);}

    throw syntax_error(std::string("Unknown token: '") + pk + "'");

    return nullptr; // Not reached
}

obj*
read(std::istream& in) {
    return basic_read(in);
}

// Return a printable represention of the object.  This a wrapper
// around `printstr()` and is mostly intended to be called from inside
// a debugger.
//
// We do this because debuggers sometimes have trouble with template
// types (e.g. std::string) so here's a function with *no* templated
// tyeps in its interface.
//
// The result is allocated on the heap so you'll need to free it if
// you use it programmaticaly.
const char *po(obj *o) {
    return po2(o, nullptr);
}

const char *po2(obj *o, const context *ctx) {
    std::string p = printstr(o, ctx).c_str();
    return strdup(p.c_str());
}


}// namespace sic
