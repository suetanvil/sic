// This file is part of Sic; Copyright (C) 2019 The Author(s)
// LGPLv2 w/ exemption; NO WARRANTY! See Copyright.txt for details

#include <unit.hpp>
#include <sic.hpp>

namespace sic {

static void
incr(context *ctx, const std::string& varname) {
    double value = dca<number>(ctx->get(varname))->val;
    value += 1;
    ctx->set(varname, new number(trunc(value)));
}// incr

int
tests_run(context *ctx) {
    double value = dca<number>(ctx->get("TEST_COUNT"))->val;
    return (long)trunc(value);
}// tests_run


long
failures(context *ctx) {
    double failures = dca<number>(ctx->get("TEST_FAILURE_COUNT"))->val;
    return (long)trunc(failures);
}// failures


bool
success(context *ctx) {
    if (!tests_run(ctx)) { return false; }

    double exp = dca<number>(ctx->get("TEST_EXPECTED_FAILURES"))->val;
    return (long)trunc(exp) == failures(ctx);
}// success



static obj*
assert_eq_helper(const std::vector<obj*>& args, context* ctx, bool equal) {
    obj *left = eval(args[0], ctx);
    obj *right = eval(args[1], ctx);
    if (equal == left->equals(right)) { return t; }

    std::string msg = equal ? "Expecting" : "Not expecting";
    msg +=  " '" + printstr(left) + "';  got '" +
        printstr(right) + "'.";
    if (args.size() > 2) { msg += " " + printstr(args[2]); }

    throw assertion_failure(msg);
    return nil;     // Not reached
}


static obj*
assert_bool(const std::vector<obj*>& args, context* ctx, bool wantTrue) {
    if (eval(args[0], ctx)->isTrue() == wantTrue) { return t; }

    std::string msg = "Expression: '" + printstr(args[0]) + "'.";
    if (args.size() > 1) { msg += " " + printstr(args[1]); }

    throw assertion_failure(msg);
    return nil;     // Not reached
}// assert_bool


void
add_test_functions(context *ctx) {
    //static string * const empty = new string("");
    static number * const zero = new number(0l);

    // These get called at macro-expansion time

    // (assert-eq? expected result ["desc"])
    static callable * const assert_eq_p =
        new builtin(
            2, true, true,
            [](std::vector<obj*> args, context* ctx) -> obj* {
                return assert_eq_helper(args, ctx, true);
            });

    // (assert-ne? non-expected result ["desc"])
    static callable * const assert_ne_p =
        new builtin(
            2, true, true,
            [](std::vector<obj*> args, context* ctx) -> obj* {
                return assert_eq_helper(args, ctx, false);
            });

    // (assert-true cond ["desc"])
    static callable * const assert_true =
        new builtin(
            1, true, true,
            [](std::vector<obj*> args, context *ctx) -> obj* {
                return assert_bool(args, ctx, true);
            });

    // (assert-false cond ["desc"])
    static callable * const assert_false =
        new builtin(
            1, true, true,
            [](std::vector<obj*> args, context *ctx) -> obj* {
                return assert_bool(args, ctx, false);
            });


    // (test "desc" expr1 expr2 ... )
    //
    // Hack alert: this is actually a special form in that it claims
    // to be a macro but does all of the processing in-place,
    // returning either 1 or nil.
    static callable * const test_form =
        new builtin(
            1, true, true,
            [](std::vector<obj*> args, context* ctx) -> obj* {
                incr(ctx, "TEST_COUNT");

                std::string message = "";

                bool first = true;
                for (obj* arg : args) {
                    if (first && arg->isString()) {
                        message = dca<string>(arg)->contents;
                        first = false;
                        continue;
                    }
                    first = false;

                    try {
                        eval(arg, ctx);
                    } catch (const assertion_failure& e) {
                        incr(ctx, "TEST_FAILURE_COUNT");
                        printf("FAILED %d %s %s\n", tests_run(ctx),
                               message.c_str(), e.msg().c_str());
                        return nil;
                    }
                }
                printf("%03d PASSED\n", tests_run(ctx));
                return t;
            });

    ctx->define("TEST_COUNT", zero);
    ctx->define("TEST_FAILURE_COUNT", zero);
    ctx->define("TEST_EXPECTED_FAILURES", zero);

    ctx->define("assert-eq?", assert_eq_p);
    ctx->define("assert-ne?", assert_ne_p);
    ctx->define("assert-true", assert_true);
    ctx->define("assert-false", assert_false);

    ctx->define("test", test_form);

}// add_test_functions


};
