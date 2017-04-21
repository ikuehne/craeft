CC=clang++
CXX=clang++

BOOST_OPT=/usr/local/Cellar/boost/1.62.0/lib/libboost_program_options.a
CPPFLAGS=-g $(shell llvm-config --cxxflags) -Wall -Wpedantic -std=c++14 -UNDEBUG
LDFLAGS=$(shell llvm-config --ldflags --system-libs --libs all) $(BOOST_OPT)
COMPILER_OBJS=Parser.o AST.o Lexer.o Error.o

all: expr_parse_demo statement_parse_demo toplevel_parse_demo

.PHONY: docs
docs:
	doxygen

expr_parse_demo: $(COMPILER_OBJS) expr_parse_demo.o
statement_parse_demo: $(COMPILER_OBJS) statement_parse_demo.o
toplevel_parse_demo: $(COMPILER_OBJS) toplevel_parse_demo.o

clean:
	$(RM) *.o expr_parse_demo
	$(RM) -r docs
