CC=clang++
CXX=clang++

BOOST_OPT=/usr/local/Cellar/boost/1.62.0/lib/libboost_program_options.a
CPPFLAGS=-g $(shell llvm-config --cxxflags) -Wall -Wpedantic -std=c++14 -UNDEBUG
LDFLAGS=$(shell llvm-config --ldflags --system-libs --libs all) $(BOOST_OPT)
COMPILER_OBJS=Parser.o Expression.o Lexer.o

all: expr_parse_demo

doc:
	doxygen

expr_parse_demo: $(COMPILER_OBJS) expr_parse_demo.o

clean:
	$(RM) *.o expr_parse_demo
	$(RM) -r doc
