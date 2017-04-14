#include <iostream>

#include "Parser.hh"

int main(int argc, char **argv) {
    std::string fname = argv[1];

    auto parser = Compiler::Parser(fname);

    try {
        Compiler::AST::print_expr(parser.parse_expression(), std::cout);
        std::cout << std::endl;
    } catch (const char *msg) {
        std::cerr << "Error: " << msg << std::endl;
    }
}
