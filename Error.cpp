#include "Error.hh"

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define TERM_ERR   "\x1b[31;1m"
#define TERM_IND   "\x1b[32;1m"
#define TERM_RESET "\x1b[0m"

namespace Craeft {

/**
 * @brief Cache of vectors containing the lines in files read so far.
 *
 * Maps filenames to vectors of lines in the file.
 */
static std::map<std::string, std::unique_ptr<std::vector<std::string>>>
    files_read;

/**
 * @brief Get the vector of lines from the given filename.
 */
static std::vector<std::string> &get_lines(std::string f) {
    if (files_read.count(f)) {
        return *files_read[f];
    } else {
        auto v = std::make_unique<std::vector<std::string>>();
        std::ifstream file(f);
        char buf[81];
        while (!file.eof()) {
            unsigned i;
            char c;
            for (i = 0; i < 80; ++i) {
                if (file.eof()) break;
                c = file.get();
                if (c == '\n') {
                    break;
                }
                buf[i] = c;
            }
            if (c != '\n') {
                while (!file.eof() && file.get() != '\n');
            }
            buf[i] = '\0';
            v->push_back(std::string(buf));
        }
        files_read[f] = std::move(v);
        return *files_read[f];
    }
}

Error::Error(std::string header, std::string msg,
             std::string fname, SourcePos pos)
    : header(header), msg(msg), fname(fname), pos(pos) {}

void Error::emit(std::ostream &out) {
    const std::vector<std::string> &lines = get_lines(fname);

    assert(pos.lineno <= lines.size());
    out << fname
        << ":" << pos.lineno << ":" << pos.charno + 1
        << ": " << TERM_ERR << header << ": " << TERM_RESET
        << msg << "\n\t"
        << lines[pos.lineno] << "\n\t"
        << std::string(pos.charno, ' ')
        << TERM_IND << "^" << TERM_RESET << std::endl;
}

}
