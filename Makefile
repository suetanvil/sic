

#CXX=g++
#LD=g++	
CXX=clang++
LD=clang++

CXXFLAGS=-Wall -Wmissing-prototypes -g -O0 -std=c++17


SRC=sic.cpp repl.cpp
OBJ=$(SRC:.cpp=.o)

ALLSRC=$(SRC) run0.cpp
ALLOBJ=$(ALLSRC:.cpp=.o)

SIC=sic.bin

all: $(SIC) run0.bin

$(SIC): $(OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(SIC)

run0.bin : $(ALLOBJ)
	$(LD) $(LDFLAGS) run0.o sic.o $(LIBS) -o run0.bin

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $<

clean:
	-rm *.o $(SERVER) $(ADMTOOL) $(SYNC) $(ARGTEST) deps.mk

tags: $(SRC)
	etags *.[ch]

# Convenience rule for launching tests from emacs
# test:
# 	(cd ../test; make test)

deps.mk:
	$(CXX) -MM $(DEFS) $(ALLSRC) > deps.mk

include deps.mk
