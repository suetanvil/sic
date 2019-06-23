
.SUFFIXES: .inc

CXX=clang++
LD=clang++
#CXX=g++
#LD=g++	

#CXXDEBUG=-g -O0 -fstandalone-debug
#CXXFLAGS=-Wall -Wmissing-prototypes $(CXXDEBUG) -std=c++17 -I. -O

CXXDEBUG=-g -O
CXXFLAGS=-Wall $(CXXDEBUG) -std=c++17 -I. -O


LIBSRC=sic.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)

REPLSRC=repl.cpp unit.cpp
REPLOBJ=$(REPLSRC:.cpp=.o)

ALLSRC=$(LIBSRC) $(REPLSRC)
ALLOBJ=$(ALLSRC:.cpp=.o)

REPL=sic

SICLIB=libsic.a

DOCFILE=function-reference.md

all: test doc

test: bin
	../tests/test_runner.sh

test_verbose: bin
	../tests/test_runner.sh --verbose

bin: $(REPL) lib

lib: $(SICLIB)

doc: $(DOCFILE)

examples: test
	( cd ../examples; \
	  for d in *.cpp; do \
	      echo $$d ; \
	  	  $(CXX) $(CXXFLAGS) $(LDFLAGS) -I../src $$d ../src/$(SICLIB) \
				-o $${d%.cpp}.bin ; \
	  done )

$(REPL): $(REPLOBJ) $(SICLIB)
	$(LD) $(LDFLAGS) $(REPLOBJ) $(SICLIB) $(LIBS) -o $(REPL)

$(SICLIB): $(LIBOBJ)
	( [ ! -f "$(SICLIB)" ] || rm "$(SICLIB)" )
	ar rcs $(SICLIB) $(LIBOBJ)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $<

clean:
	-rm *.o $(SICLIB) $(REPL) deps.mk $(DOCFILE)
	-(cd ../examples; rm *.o *.bin; rm -rf *.dSYM)

$(DOCFILE) : sic_func.inc
	perl ../scripts/make-doc.pl sic_func.inc > $(DOCFILE)

tags: $(ALLSRC)
	etags *.[ch]pp *.inc

# Convenience rule for launching tests from emacs
# test:
# 	(cd ../test; make test)

#$(ALLOBJ) : sic_func.inc

deps.mk:
	$(CXX) -I. -MM $(DEFS) $(ALLSRC) > deps.mk

include deps.mk
