/**
 * @file expr_parse_demo.cpp
 *
 * A demo/test of the expression parser.  To be deleted once the parser is
 * completed and more systematic testing is added.
 */

#include <iostream>

#include "Parser.hh"

int main(int argc, char **argv) {
    std::string fname = argv[1];

    auto parser = Craeft::Parser(fname);

    try {
        auto tl = parser.parse_toplevel();
        Craeft::AST::print_toplevel(tl, std::cout);
        std::cout << std::endl;
        return 0;
    } catch (const char *msg) {
        std::cerr << "Error: " << msg << std::endl;
    } catch (std::string msg) {
        std::cerr << "Error: " << msg << std::endl;
    } catch (Craeft::Error err) {
        err.emit(std::cerr);
    }

    return 1;
}
