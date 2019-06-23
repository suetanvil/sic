
#include <sic.hpp>

using namespace sic;

static obj *
code() {
    return
        $(progn,

          $(print, "starting!\n"),

          $(defun, $$("fib"), $( $$("n") ),
            $(if_op, $(le, $$("n"), 1),
              $(list, 1),
              $(if_op, $(eq_p, $$("n"), 2),
                $(list, 1, 1),
                $(let, $( $( $$("prev"), $( $$("fib"), $(sub, $$("n"), 1) ) ) ),
                  $(pair_op, $(add, $(first, $$("prev")), $(second, $$("prev"))), $$("prev") )
                    )
                  )
                )
              ),

          $(print,
            $( $$("fib"), $(str_to_num, $(third, $$("argv")) ) ),
            "\n")
            )
        ;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number>\n", argv[0]);
        exit(1);
    }

    context* root = root_context();

    obj *sic_argv = $("", "", argv[1]);
    root->set("argv", sic_argv);

    eval(code(), root);
    return 0;
}
