/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "CommandLine.hpp"

#include <boost/program_options.hpp>

namespace impl_detail {

CommandLine::cl_t reconstruct_command_line(const CommandLine::tokens_t& tokens, const std::string& separator = " ") {
    std::ostringstream cl;
    if (!tokens.empty()) {
        cl << tokens.front();
        for (size_t i = 1; i < tokens.size(); ++i) {
            cl << separator << tokens[i];
        }
    }
    return cl.str();
}

CommandLine::tokens_t reconstruct_tokens(int argc, const char** argv) {
    CommandLine::tokens_t tokens;
    for (int i = 0; i < argc; ++i) {
        tokens.push_back(argv[i]);
    }
    return tokens;
}

} // namespace impl_detail

CommandLine::CommandLine(int argc, char** argv) : CommandLine(argc, const_cast<const char**>(argv)) {
}

CommandLine::CommandLine(int argc, const char** argv) : tokens_{impl_detail::reconstruct_tokens(argc, argv)} {
}

CommandLine::CommandLine(std::string cl) : tokens_{boost::program_options::split_unix(cl)} {
}

CommandLine::CommandLine(tokens_t tokens) : tokens_{std::move(tokens)} {}

CommandLine::cl_t CommandLine::original() const {
    return impl_detail::reconstruct_command_line(tokens_);
}

size_t CommandLine::size() const {
    return tokens_.size();
}

const CommandLine::tokens_t& CommandLine::tokens() const {
    return tokens_;
}

std::ostream& operator<<(std::ostream& os, const CommandLine& cl) {
    os << "argc=" << cl.tokens_.size() << ", argv=[" << impl_detail::reconstruct_command_line(cl.tokens_, ", ") << "]";
    return os;
}
