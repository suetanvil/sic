
#include "sic.hpp"

using namespace sic;

static void
pr(obj *exp) {
    context ctx(nullptr);
    printf("%s\n", printstr(eval(exp, &ctx)).c_str());
}// pr

static void
pq(obj *exp) {
    printf("%s\n", printstr(exp).c_str());
}// pq



int main() {
    try {
        pr(     $(add, 1, 2.0, 3)     );
        pr(     $(sub, 42, 20.9)      );
        pr(     $(mul, 6, 3)        );
        pr(     $(divi, 12, 2)       );

#if 1
        pr(     $(progn, 1, 2, 3, 4, 5) );
        pr(
            $(progn,
              $(set, $(quote, $$("foo")), 42),
              99,
              $(quote,$$("foo")),
              $$("foo")
                )
            );
        pr(
            $(progn,
              $(set,
                $(quote, $$("f")),
                $(lambda,
                  $( $$("n") ),
                  $(add, $$("n"), 1)  )),
              
              $($$("f"), 42)
                ));
        pr(
            $(quote, $$("foobar"))
            );

        pr(
            $(quote2, $($$("a"), $$("b"), $$("c")),
              $($$("d"), $$("e"), $$("f")))
            );
#endif
        
        pr(
            
            $(progn,
              $(setq, $$("one"), 1),
              $(setq, $$("addone"),
                $(lambda, $( $$("n") ), $(add, $$("n"), $$("one")))),
              $($$("addone"), 42))
            
            );

        pq( read( "(1 2.0 foo \"bar\")" ) );
        pq( read( "(1 2.0 (+ 3 4) --- x)" ) );
        pq( read( "(1 2.0 '(+ 3 4) --- x)" ) );
        pq( read( "(\"\" \"abcde\" \"1\")" ) );

        
    } catch(error& e) {
        printf("Exception: %s\n", e.msg().c_str());
    }

    return 0;
}
