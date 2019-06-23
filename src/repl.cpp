// This file is part of Sic; Copyright (C) 2019 The Author(s)
// LGPLv2 w/ exemption; NO WARRANTY! See Copyright.txt for details

#include "sic.hpp"
#include "unit.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <memory>


using namespace sic;

static bool
is_test(const std::string& name) {
    const std::string ending = ".sictest";

    if(name.length() < ending.length()) { return false; }

    return 0 == name.compare(name.length() - ending.length(), ending.length(),
                             ending);
}// is_test


static obj*
read_and_eval(bool *keepGoing, context *ctx) {
    try {
        std::string line = "";
        std::getline(std::cin, line);

        obj* expr = read(line);
        if (!expr) { return nullptr; }

        return eval(expr, ctx);
    } catch(const error& e) {
        std::cout << "ERROR: " << e.msg() << "\n";
    }

    return nullptr;
}// read_and_eval


static void
repl() {
    const std::unique_ptr<context> root(root_context());
    bool go = true;

    while(go) {
        if (!std::cin.good()) { return; }

        std::cout << "> ";
        std::cout.flush();

        obj *result = read_and_eval(&go, root.get());
        if (result && result != nil) {
            std::cout << printstr(result) << "\n";
        }// if
    }// while
}// repl

static int
run_script(const std::string& path, obj *argv) {
    context* root = root_context();

    std::fstream in;
    in.open(path);

    if (!in.is_open()) {
        std::cerr << "Unable to open '" << path << "'\n";
        exit(2);
    }// if

    bool testmode = is_test(path);
    try {

        if (testmode) {
            add_test_functions(root);
        }

        // Set the argument list.
        root->set("argv", argv);

        while (in.good()) {
            obj *expr = read(in);
            if (!expr) { break; }

            eval(expr, root);
        }// while

        if (testmode) {
            if (tests_run(root) == 0) {
                std::cerr << "ERROR: test script '" << path
                          << "' contains no tests.\n";
                return 1;
            }

            std::cout << "Ran " << tests_run(root) << " test(s)\n";

            if (!success(root)) {
                long f = failures(root);
                std::cout << f << " tests failed!\n";
                return f;
            }// if

            if (failures(root) > 0) {
                std::cout << "(All failures were expected.)\n";
            }// if
        }// if

        return 0;
    } catch (const error& e) {
        std::cerr << "ERROR: " << e.longmsg() << "\n";
        return 1;
    }// catch

    return 0;       // Not reached
}// run_script



static obj *
argv_list(int argc, char *argv[]) {
    std::vector<obj*> avobj(argc);
    std::transform(argv, argv+argc, avobj.begin(),
                   [](const char *s) -> obj* {
                       return new string(std::string(s));
                   });

    return vec2list(avobj);
}


int
main(int argc, char *argv[]) {
    try {
        if (argc >= 2) {
            return run_script(argv[1], argv_list(argc, argv));
        } else {
            repl();
        }// if .. else
    } catch (sic::error& e) {
        std::cout << "Uncaught sic exception: " << e.msg() << "\n";
        return 2;
    }// catch

    return 0;
}
