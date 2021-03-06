#include <cassert>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <boost/program_options.hpp>
#include <boost/variant.hpp>

#include "Parser.hh"
#include "Codegen/Module.hh"

namespace opt = boost::program_options;

/* Actually the privileges most compilers create object files with. */
static const int OBJFILE_MODE_BLAZEIT = 420;

/**
 * @brief Pull a single AST out of the parser, and have the code generator
 *        visit it.
 */
bool handle_input(Craeft::Parser &p, Craeft::Codegen::ModuleGen &c) {
    try {
        auto e = p.parse_toplevel();
        c.codegen(*e);
        return true;
    } catch (Craeft::Error e) {
        e.emit(std::cerr);
        return false;
    }
}

/**
 * @brief Entry point.
 */
int main(int argc, char **argv) {
    /* Boost command-line option stuff... */
    opt::options_description desc("Craeft Compiler Options");
    desc.add_options()
        ("help", "print usage information")
        ("obj,c", opt::value<std::string>(),
            "select output file to emit object code")
        ("ll", opt::value<std::string>(),
            "select output file to emit LLVM IR")
        ("asm,s", opt::value<std::string>(),
            "select output file to emit target-specific assembly")
        ("opt,O", opt::value<int>()->default_value(0),
            "select optimization level (default 0)")
        ("in", opt::value<std::string>(), "select input file");
    opt::positional_options_description pos;
    pos.add("in", -1);

    opt::variables_map opt_map;
    try {
        opt::store(opt::command_line_parser(argc, argv)
                      .options(desc)
                      .style(boost::program_options::command_line_style
                                  ::unix_style)
                      .positional(pos)
                      .run(),
                   opt_map);
    } catch (opt::error) {
        std::cerr << desc << std::endl;
        return 1;
    }
    opt::notify(opt_map);

    int opt_level = opt_map["opt"].as<int>();

    /* If the user did good, */
    if (!opt_map.count("help")
      && (opt_map.count("obj") || opt_map.count("ll") || opt_map.count("asm"))
      && opt_map.count("in")) {
        auto in_file = opt_map["in"].as<std::string>();
        /* Get a code generator. */
        Craeft::Codegen::ModuleGen codegen("Craeft module", in_file);
        /* Construct a parser on that file. */
        Craeft::Parser parser(in_file);
        bool successful = true;
        /* Pull ASTs out of the parser */
        while (successful) {
            /* until we hit EOF. */
            if (parser.at_eof()) break;
            if (!handle_input(parser, codegen)) successful = false;
        }

        if (!successful) return 2;

        /* Validate the module. */
        codegen.validate(std::cerr);
        /* Optimize the module to the chosen level. */
        codegen.optimize(opt_level);

        if (opt_map.count("obj")) {
            /* Open the output file (LLVM's stream formats are weird, so we
             * can't use regular STL stream classes). */
            int fd = open(opt_map["obj"].as<std::string>().c_str(),
                          O_RDWR | O_CREAT,
                          OBJFILE_MODE_BLAZEIT);
            /* Emit the object code. */
            codegen.emit_obj(fd);
            close(fd);
        }
        if (opt_map.count("asm")) {
            int fd = open(opt_map["asm"].as<std::string>().c_str(),
                          O_RDWR | O_CREAT,
                          OBJFILE_MODE_BLAZEIT);
            /* Emit the assembly code. */
            codegen.emit_asm(fd);
            close(fd);
        }
        if (opt_map.count("ll")) {
            std::ofstream file(opt_map["ll"].as<std::string>());
            codegen.emit_ir(file);
        }
    } else {
        /* Print usage information if the user did bad. */
        std::cerr << desc << std::endl;
        return 1;
    }

}
