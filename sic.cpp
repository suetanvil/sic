
#include <string>
#include <exception>
#include <map>
#include <vector>
#include <istream>

#include "sic.hpp"

namespace sic {


pair * const nil = nilClass::getInstance();
pair * const null = nil;     // 'cuz nil is reserved by lldb

callable * const add = new builtin(
    [](std::vector<obj*> args, context*) {
        double sum = 0;
        for(obj* i : args) {
            sum += dca<number>(i)->val;
        }

        return new number(sum);
    }, 2, true);

callable * const sub = new builtin(
    [](std::vector<obj*> args, context*) {
        return new number(dca<number>(args[0])->val - dca<number>(args[1])->val);
    }, 2);

callable * const mul = new builtin(
    [](std::vector<obj*> args, context*) {
        return new number(dca<number>(args[0])->val * dca<number>(args[1])->val);
    }, 2);

callable * const divi = new builtin(
    [](std::vector<obj*> args, context*) {
        return new number(dca<number>(args[0])->val / dca<number>(args[1])->val);
    }, 2);

callable * const make_function = new builtin(
    [](std::vector<obj*> args, context* ctx) {
        pair *formals = dca<pair>(args[0]);
        pair *body = dca<pair>(args[1]);
        bool isLambda = args[2] != nil;
        bool isMacro = args[3] != nil;


        return new function(formals, body,
                            isLambda ? ctx : ctx->root(), isMacro);
    }, 4);

callable * const print = new builtin(
    [](std::vector<obj*> args, context*) {
        for(obj* i : args) {
            printf("%s", printstr(i).c_str());
        }

        return nil;
    }, 1, true);

callable * const set = new builtin(
    [](std::vector<obj*> args, context* ctx) {
        ctx->tl_set(dca<symbol>(args[0])->text, args[1]);
        return args[1];
    }, 2);

callable * const progn = new builtin(
    [](std::vector<obj*> args, context* ctx) {
        return (args.size() == 0) ? nil : args.back();
    }, 0, true);

#if 0
callable * const apply = new builtin(
    [](std::vector<obj*> args, context* ctx) {
//        return eval(dca<callable>(args[0]),
    }, 2, false);

callable * const defmacro_fn = new builtin(
    [](std::vector<obj*> args, context* ctx) {
        return (args.size() == 0) ? nil : args.back();
    }, 2, false, true);
#endif

//
// Macros:
//

// This will probably never get called, but we need it for pointer
// identity.
callable * const quote = new builtin(
    [](std::vector<obj*> args, context*) { return args[0]; },
    1, false, true);


// This gets used for testing only
callable * const quote2 = new builtin(
    [](std::vector<obj*> args, context*) {
        return $(quote, $(args[0], args[1]));
    },
    2, false, true);

// Lambda macro
callable * const lambda = new builtin(
    [](std::vector<obj*> args, context*) {
        pair *formals = dca<pair>(args[0]);
        args.erase(args.begin());

        pair *body = nil;
        for (auto i = args.rbegin(); i != args.rend(); ++i) {
            body = new pair(*i, body);
        }

        return $(make_function, $(quote, formals), $(quote, body), 1, nil);
    }, 2, true, true);

/*
// (defun foo (a1 a2 a3) ... )
callable * const defun = new builtin(
    [](std::vector<obj*> args, context*) {
        symbol *name = dca<symbol>(args[0]);
        args.erase(args.begin());

        pair *formals = dca<pair>(args[0]);
        args.erase(args.begin();

        pair *body = nil;
        for (auto i = args.rbegin(); i != args.rend(); ++i) {
            body = new pair(*i, body);
        }
     return $(make_function, $(quote, formals), $(quote, body), 1, nil);

    }, 2, true, true));
*/

callable * const setq = new builtin(
    [](std::vector<obj*> args, context*) {
        return $(set, $(quote, args[0]), args[1]);
    },
    2, false, true);





std::size_t llen(pair *lst) {
    std::size_t len = 1;

    for (pair *cel = lst; cel->cdr != nil; cel = dca<pair>(cel->cdr)) {
        ++len;
        if (cel->cdr->isAtom()) { throw wrong_type("list", printstr(cel->cdr));}
    }

    return len;
}// llen


static obj *
reverse_helper(obj *list, obj *reversed) {
    if (list == nil) { return reversed; }

    pair *plist = dca<pair>(list);
    return reverse_helper(plist->cdr, new pair(plist->car, reversed));
}

pair *
reverse(pair *list) {
    return dca<pair>(reverse_helper(list, nil));
}// reverse


// Evaluate one expression.
obj*
eval(obj* expr, context* ar) {

    if (expr->isSymbol()) { return ar->get(dca<symbol>(expr)->text); }
    if (expr == nil || expr->isAtom()) { return expr; }

    if (!expr->isList())  { throw malformed_expr(); }


    pair *pexpr = dca<pair>(expr);
    pair *expr_args = dca<pair>(pexpr->cdr);

    // 'quote' is a special case
    if (pexpr->car == quote) {
        if (llen(expr_args) != 1) { throw fn_arg_mismatch(); }
        return expr_args->car;
    }

    // Retrieve the function object; this entails eval'ing the first
    // item in expr.
    obj *fun = eval(pexpr->car, ar);
    if (!fun->isCallable()) { throw not_a_function(); }

    // If this is a macro, expand it and eval() the result
    if (dca<callable>(fun)->isMacro) {
        obj* new_expr = dca<callable>(fun)->call(expr_args, ar);
        return eval(new_expr, ar);
    }// if

    // Evaluate the arguments and create a new list containing the
    // results.
    obj *actual = map(
        expr_args,
        [&](obj* item) -> obj* {
            return eval(item, ar);
        });

    return dca<callable>(fun)->call(actual, ar);
}// eval


obj*
function::call(obj* actualArgs, context*) const {
    context ctx(outer);

    pair *args = dca<pair>(actualArgs);
    assert(args);

    // Bind the argument values to their corresponding variables
    if (llen(formals) != llen(args)) { throw fn_arg_mismatch(); }
    obj *curr_arg = args;
    each(
        formals,
        [&](obj* curr) {
            ctx.define(dca<symbol>(curr)->text,
                       dca<pair>(curr_arg)->car);
            curr_arg = dca<pair>(curr_arg)->cdr;
        });

    // Evaluate the function body.
    obj *result = nil;
    each(
        body,
        [&](obj* subexpr) {
            result = eval(subexpr, &ctx);
        });

    return result;
}// call


obj*
builtin::call(obj* actualArgs, context* outer) const {
    std::vector<obj*> args;

    std::size_t naa = llen(dca<pair>(actualArgs));
    if ( (!isVariadic && naa != nargs) || naa < nargs) {
        throw arg_count(nargs, naa);
    }

    args.reserve(naa);

    for (obj *c = actualArgs; c != nil; c = dca<pair>(c)->cdr) {
        args.push_back(dca<pair>(c)->car);
    }

    return code(args, outer);
}// builtin::call


#if 0
obj* tl_eval(obj *expr) {
    context tl(nullptr);
    return eval(expr, &tl);
}
#endif

std::string printstr(obj *o) {
    if (o == quote) { return "quote"; }

    if (o->isList()) {
        std::string result = "(";

        bool first = true;
        for(obj *c = o; c != nil; c = dca<pair>(c)->cdr) {
            if (!first) { result += ' '; }
            result += printstr(dca<pair>(c)->car);
            first = false;
        }
        result += ")";

        return result;
    }

    
    return o->str();
}// printstr


void
each(obj *list, std::function<void(obj *)> actor) {
    for (obj *curr = dca<pair>(list);
         curr != nil;
         curr = dca<pair>(curr)->cdr)
    {
        actor(dca<pair>(curr)->car);
    }// for
}// each


// Convert a C++ std::vector<obj*> object to a list.
pair *
vec2list(const std::vector<obj*>& vec) {
    pair *result = nullptr;
    pair *last = nullptr;

    for (obj *item : vec) {
        pair* next = new pair(item, nil);

        if (!result) {
            result = next;
        } else {
            // We directly write next to the cdr of the previous item.
            // This violates the constness of pair, which is bad, but
            // it improves performance, which is good.
            *(const_cast<obj**>(&(last->cdr))) = next;
        }
        last = next;
    }// for

    return result;
}// vec2list


// Apply 'actor' to each item in 'list', returning a new list
// containing the result of calling 'actor'.
pair *
map(pair *list, std::function<obj*(obj *)> actor) {

    std::vector<obj*> result;

    for (pair *curr = list; curr != nil; curr = dca<pair>(curr->cdr)) {
        result.push_back(actor(curr->car));
    }// for

    return vec2list(result);
}// map


context *
root_context() {
    context *tl = new context(nullptr);

    tl->define("nil", nil);
    tl->define("null", null);

    tl->define("add", add);
    tl->define("+", add);
    tl->define("sub", sub);
    tl->define("-", sub);
    tl->define("mul", mul);
    tl->define("*", mul);
    tl->define("divi", divi);
    tl->define("/", divi);

    tl->define("print", print);

    tl->define("quote", quote);
    tl->define("make_function", make_function);
    tl->define("lambda", lambda);
    tl->define("set", set);
    tl->define("setq", setq);
    tl->define("progn", progn);

    tl->define("quote2", quote2);
    
    return tl;
}




#if 1

static obj *basic_read(std::istream& in);


static void
skip_spaces(std::istream& in) {
    while(in.good() && isspace(in.peek())) { in.get(); }
}



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

    char curr;
    while(true) {
        curr = in.get();
        if (in.eof() || !isdigit(curr)) { break; }

        n *= 10.0;
        n += (double)(curr - '0');
    }

    if (in.eof() || curr != '.') {
        if (!in.eof()) { in.putback(curr); }
        return new number(n);
    }

    // Handle the fractional part if present
    double pos = 10.0;
    while(true) {
        curr = in.get();
        if (in.eof() || !isdigit(curr)) { break; }

        n += (double) (curr - '0') / pos;
        pos *= 10.0;
    }// while

    return new number(n);
}// read_number


static obj *
read_word(std::istream& in, const std::function<bool(char c)>& validp) {
    std::string result = "";

    for (char c = in.peek(); !in.eof() && validp(c); c = in.peek()) {
        result += in.get();
    }// for

    return $$(result);
}// read_word

// Return null on eof
static obj *
basic_read(std::istream& in) {
    std::vector<obj*> result;  // 'cuz who wants to reverse a list?

    auto isWord = [](char c) { return isalnum(c) || c == '-' || c == '_'; };

    const static std::string ops = "-+=~!@$%^&*|\\//?<>.,:;";
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


#endif




}// namespace sic
