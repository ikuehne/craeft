/**
 * @file Scope.hh
 *
 * Scopes, as maps (optimized for small sizes) which can be pushed and popped.
 */

/* Craeft: a new systems programming language.
 *
 * Copyright (C) 2017 Ian Kuehne <ikuehne@caltech.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <boost/range/adaptor/reversed.hpp>

namespace Craeft {

class KeyNotPresentException {};

class EmptyPopException {};

template<typename T>
class Scope {
public:
    bool present(const std::string &key) const {
        for (const auto &vec: boost::adaptors::reverse(map)) {
            for (const auto &pair: vec) {
                if (pair.first == key) {
                    return true;
                }
            }
        }

        return false;
    }

    void push(void) {
        map.push_back(std::vector<std::pair<std::string, T> >());
    }

    void pop(void) {
        if (map.size()) {
            map.pop_back();
        } else {
            throw EmptyPopException();
        }
    }

    void bind(const std::string &key, const T &binding) {
        map.back().push_back(std::pair<std::string, T>(key, binding));
    }

    const T &operator[](const std::string &key) const {
        for (const auto &vec: boost::adaptors::reverse(map)) {
            for (const auto &pair: boost::adaptors::reverse(vec)) {
                if (pair.first == key) {
                    return pair.second;
                }
            }
        }

        throw KeyNotPresentException();
    }

private:
    std::vector< std::vector<std::pair<std::string, T> > >map;
};

}
