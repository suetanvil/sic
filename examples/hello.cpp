
#include <sic.hpp>

using namespace sic;

static obj *
code() {
    return 

        $(print, "Hello! World!\n")

        ;
}

int main() {
    eval(code(), root_context());
    return 0;
}
