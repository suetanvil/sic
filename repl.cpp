
#include "sic.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

using namespace sic;

#if 0
static void
pr(obj *exp) {
    printf("%s\n", printstr(tl_eval(exp)).c_str());
}// pr
#endif



static obj*
read_and_eval(bool *keepGoing, context *ctx) {
    try {
        std::string line = "";
        std::getline(std::cin, line);

//        std::stringstream s(line, std::ios_base::in);
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
        if (result) {
            std::cout << printstr(result) << "\n";
        }// if
    }// while
}// repl

static int
run_script(const std::string& path) {
    std::fstream in;
    in.open(path);

    if (!in.is_open()) {
        std::cerr << "Unable to open '" << path << "'\n";
        exit(2);
    }// if

    try {
        const std::unique_ptr<context> root(root_context());
        while (in.good()) {
            obj *expr = read(in);
            if (!expr) { break; }

            eval(expr, root.get());
        }// while 
        return 0;
    } catch (const error& e) {
        std::cerr << "ERROR: " << e.msg() << "\n";
        return 1;
    }// catch

    return 0;       // Not reached
}// run_script

int
main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cout << "Usage: sic <filename>?\n";
        return 1;
    }// if

    if (argc == 2) {
        return run_script(argv[1]);
    } else {
        repl();
    }// if .. else

    return 0;
}
