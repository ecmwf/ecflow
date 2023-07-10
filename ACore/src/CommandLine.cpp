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
#include <boost/tokenizer.hpp>

namespace impl_detail {

/**
 * The `custom_unix_style_separator` implements a TokenizerFunction (from Boost.Tokenizer), and allows to separate the
 * tokens of Unix style CLI calls. This separation is performed considering that repeated consecutive separators
 * can appear. The following is an example of the expected tokenizing:
 *   Given `   executable  --option    param  " --value "      ""   "\""  --options`, the expected tokens are:
 *     - `executable`
 *     - `--option`
 *     - `param`
 *     - ` --value `
 *     - ``
 *     - `"`
 *     - `--options`
 */
class custom_unix_style_separator {

private:
    std::string escape_    = "\\";
    std::string separator_ = " \t";
    std::string quote_     = "'\"";
    bool last_             = false;

    bool is(char e, std::string set) {
        return std::find_if(set.begin(), set.end(), [e](char c) { return c == e; }) != set.end();
    }

    bool is_escape(char e) { return is(e, escape_); }
    bool is_separator(char e) { return is(e, separator_); }
    bool is_quote(char e) { return is(e, quote_); }

    template <typename iterator>
    bool advance_separators(iterator& next, iterator& end) {

        // Advance through all separators
        while (is_separator(*next) && next != end) {
            next++;
        }

        return (next == end);
    }

    template <typename iterator, typename Token>
    void do_escape(iterator& next, iterator end, Token& tok) {
        if (++next == end) {
            throw std::runtime_error(std::string("Unexpected end of escape sequence"));
        }
        if (*next == 'n') {
            tok += '\n';
            return;
        }
        else if (is_quote(*next) || is_separator(*next) || is_escape(*next)) {
            tok += *next;
            return;
        }
        else {
            throw std::runtime_error("Unexpected escape sequence");
        }
    }

    std::vector<char> selected_quote_ = {};

    bool in_quote() { return !selected_quote_.empty(); }

public:
    void reset() { last_ = false; }

    template <typename InputIterator, typename Token>
    bool operator()(InputIterator& next, InputIterator end, Token& tok) {
        // Reset collected Token
        tok        = Token();
        bool valid = false;

        // Consume pre-separators, and handle the case where we reach the end of the iterable
        if (advance_separators(next, end)) {
            if (last_) {
                last_ = false;
                return true;
            }
            return false;
        }

        // Otherwise, collect the token
        last_ = false;
        for (; next != end; ++next) {
            if (is_escape(*next)) {
                do_escape(next, end, tok);
            }
            else if (is_separator(*next)) {
                if (!in_quote()) {
                    // If we are not in quote, then we are done
                    ++next;
                    // The last character was a c, that means there is
                    // 1 more blank field
                    last_ = true;

                    // Consume post-separators
                    if (advance_separators(next, end)) {
                        if (last_) {
                            last_ = false;
                            return true;
                        }
                        return false;
                    }

                    return true;
                }
                else {
                    valid = true;
                    tok += *next;
                }
            }
            else if (is_quote(*next)) {

                if (!in_quote()) {
                    // Entering quoted sequence...
                    selected_quote_.push_back(*next);
                }
                else if (selected_quote_.back() == *next) {
                    // Matching quotation marks detected... Ending the quoted sequence!
                    selected_quote_.pop_back();
                }
                else {
                    // Unmatched quotation marks detected... Keep collecting the quoted sequence!
                    tok += *next;
                }
            }
            else {
                valid = true;
                tok += *next;
            }
        }

        // Consume post-separators
        if (advance_separators(next, end)) {
            if (last_) {
                last_ = false;
                return true;
            }
            if (in_quote()) {
                throw std::runtime_error("Unmatched quotation marks detected");
            }
            return valid;
        }

        if (in_quote()) {
            throw std::runtime_error("Unmatched quotation marks detected");
        }
        return true;
    }
};

CommandLine::cl_t reconstruct_command_line(const CommandLine::tokens_t& tokens, const std::string& separator = " ") {
    std::ostringstream cl;
    if (!tokens.empty()) {
        cl << tokens.front();
        for (size_t i = 1; i < tokens.size(); ++i) {
            cl << separator << '"' << tokens[i] << '"';
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

std::vector<std::string> split_unix(const std::string& cmdline) {
    using separator_t = custom_unix_style_separator;
    using tokenizer_t = boost::tokenizer<separator_t, std::string::const_iterator, std::string>;

    tokenizer_t tok(std::begin(cmdline), std::end(cmdline), separator_t());

    std::vector<std::string> result;
    for (typename tokenizer_t::iterator curT(tok.begin()), endT(tok.end()); curT != endT; ++curT) {
        result.push_back(*curT);
    }
    return result;
}

} // namespace impl_detail

CommandLine::CommandLine(int argc, char** argv) : CommandLine(argc, const_cast<const char**>(argv)) {
}

CommandLine::CommandLine(int argc, const char** argv) : tokens_{impl_detail::reconstruct_tokens(argc, argv)} {
}

CommandLine::CommandLine(const std::string& cl) : tokens_{impl_detail::split_unix(cl)} {
}

CommandLine::CommandLine(tokens_t tokens) : tokens_{std::move(tokens)} {
}

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
