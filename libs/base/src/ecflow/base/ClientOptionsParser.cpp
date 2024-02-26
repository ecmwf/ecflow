/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/ClientOptionsParser.hpp"

#include "ecflow/core/Str.hpp"

namespace ecf {

namespace {

bool is_valid_path(const std::string& path) {
    return !path.empty() && path[0] == '/';
}

} // namespace

ClientOptionsParser::option_set_t ClientOptionsParser::operator()(ClientOptionsParser::arguments_set_t& args) {
    option_set_t processed_options;

    // *** Important! ***
    // This custom handler is needed to ensure that the "--alter" option
    // special value parameters are handled correctly. For example,
    // values starting with --, such as "--hello".
    //
    // The custom handling will consider that 5+ positional values (not
    // to be confused with positional arguments) are provided with the
    // --alter option, as per one of the following forms:
    //     1) --alter arg1 arg2 arg3 (arg4) path [path [path [...]]]
    //     2) --alter=arg1 arg2 arg3 (arg4) path [path [path [...]]]

    if (ecf::algorithm::starts_with(args[0], "--alter")) {

        option_t alter{std::string{"alter"}, {}};

        if (auto found = args[0].find('='); found == std::string::npos) {
            // In case form 1) is used, we discard the '--alter'
            args.erase(args.begin());
            // store the 'arg1'
            alter.value.push_back(args.front());
            alter.original_tokens.push_back(args.front());
        }
        else {
            // Otherwise, form 2) is used, and the first positional value must be
            // taken by tokenizing the option '--alter=arg1'
            std::string arg = args[0].substr(found + 1);
            alter.value.push_back(arg);
            alter.original_tokens.push_back(arg);
        }
        // Discard the option at the front
        args.erase(args.begin());

        // Collect (non path) positional arguments
        const size_t maximum_positional_args = 4;
        for (size_t i = 0; i < maximum_positional_args && !args.empty(); ++i) {
            // Take each of the positional values as an option value
            std::string& arg = args.front();
            // Once we find the first path argument we stop collecting arguments
            if (is_valid_path(arg)) {
                break;
            }
            alter.value.push_back(arg);
            alter.original_tokens.push_back(arg);

            // Remove the argument value
            args.erase(args.begin());
        }

        // Collect only paths
        for (; !args.empty();) {
            // Take each of the positional values as an option value
            std::string& arg = args.front();
            // Once we find a (non path) argument, we stop collections arguments
            if (!is_valid_path(arg)) {
                break;
            }
            alter.value.push_back(arg);
            alter.original_tokens.push_back(arg);

            // Remove the argument value
            args.erase(args.begin());
        }

        processed_options.push_back(alter);
    }
    return processed_options;
}

} // namespace ecf
