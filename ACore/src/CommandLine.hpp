/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef CORE_COMMANDLINE_HPP
#define CORE_COMMANDLINE_HPP

#include <ostream>
#include <string>
#include <vector>

/**
 * CommandLine is a thin wrapper/container for the contents of an ecFlow client command line.
 *
 * Note: This class is related to class ArgvCreator, which takes the vector of the command
 * line tokens and converts them into argc+argv.
 */
class CommandLine {
public:
    using cl_t     = std::string;
    using tokens_t = std::vector<std::string>;
    using size_t   = tokens_t::size_type;

public:
    /**
     * Create a Command Line instance based on argc + argv
     *
     * @param argc the number of tokens in the command line
     * @param argv the tokens in the command line
     */
    CommandLine(int argc, char** argv);
    CommandLine(int argc, const char** argv);

    /**
     * Create a Command Line instance based on a sequence of characters,
     * considering that it might include single/double quotes.
     *
     * @param cl the command line as a sequence of characters
     */
    explicit CommandLine(const cl_t& cl);

    /**
     * Create a Command Line instance based on a sequence of tokens.
     *
     * @param tokens the command line as a sequence of tokens
     */
    explicit CommandLine(tokens_t tokens);

    /**
     * Create a Command Line instance based on multiple tokens.
     * Tokens can be either strings or collections of strings. In the latter case, the collection of strings are
     * added to the command line element-by-element.
     *
     * @param tokens the command line as multiple tokens
     */
    template <typename... TOKENS>
    static CommandLine make_command_line(TOKENS... tokens) {
        auto push = Overload{
            [](std::vector<std::string>& collection, const std::vector<std::string>& tokens) {
                std::copy(std::begin(tokens), std::end(tokens), std::back_inserter(collection));
            },
            [](std::vector<std::string>& collection, const std::string& token) {
                collection.push_back(token);
            }};

        std::vector<std::string> ts;
        ((push(ts, tokens)), ...);
        return CommandLine(ts);
    }

    /**
     * Access the original content of the command line.
     */
    [[nodiscard]] cl_t original() const;

    /**
     * Access the number of tokens composing the command line.
     *
     * @return the number of tokens
     */
    [[nodiscard]] size_t size() const;

    /**
     * Access the sequence of tokens composing the command line.
     *
     * @return the command line tokens
     */
    [[nodiscard]] const tokens_t& tokens() const;

    /**
     * Pretty print the command line
     */
    friend std::ostream& operator<<(std::ostream& os, const CommandLine& cl);

private:
    tokens_t tokens_;

    // Utility to allow the combination of multiple lambdas
    template <typename... L>
    struct Overload : L...
    {
        using L::operator()...;
        constexpr explicit Overload(L... lambda) : L(std::move(lambda))... {}
    };
};

#endif
