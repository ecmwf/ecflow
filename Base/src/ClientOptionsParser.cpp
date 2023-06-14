/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ClientOptionsParser.hpp"

#include <boost/algorithm/string.hpp>

namespace ecf {

ClientOptionsParser::option_set_t ClientOptionsParser::operator()(ClientOptionsParser::arguments_set_t& args) {
    option_set_t processed_options;

    // *** Important! ***
    // This custom handler is needed to ensure that the "--alter" option
    // special value parameters are handled correctly. For example,
    // values starting with --, such as "--hello".
    //
    // The custom handling will consider that 5 positional values (not
    // to be confused with positional arguments) are provided with the
    // --alter option, as per one of the following forms:
    //     1) --alter arg1 arg2 arg3 arg4 arg5
    //     2) --alter=arg1 arg2 arg3 arg4 arg5

    if (boost::algorithm::starts_with(args[0], "--alter")) {

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

        for (size_t i = 1; i < 5 && !args.empty(); ++i) {
            // Take each of the 5 mandatory positional values as an option value
            std::string& arg = args.front();
            alter.value.push_back(arg);
            alter.original_tokens.push_back(arg);

            // Remove the positional value
            args.erase(args.begin());
        }
        processed_options.push_back(alter);
    }
    return processed_options;
}

} // namespace ecf
