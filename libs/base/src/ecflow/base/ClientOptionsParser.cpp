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

void parse_option(ClientOptionsParser::option_t& option,
                  ClientOptionsParser::option_set_t& processed_options,
                  ClientOptionsParser::arguments_set_t& args) {
    // We must consider two forms of options:
    //     1) --<option> <arg1>
    //     2) --<option>=<arg1>

    if (auto found = args[0].find('='); found == std::string::npos) {
        // In case form 1) is used, we discard the '--<option>'
        args.erase(args.begin());
        if (args.empty()) {
            // This means that the option doesn't actually have a value (i.e. acts as a flag) and we simply return
            return;
        }
        // store the 'arg1'
        option.value.push_back(args.front());
        option.original_tokens.push_back(args.front());
    }
    else {
        // Otherwise, form 2) is used, and the first positional value must be
        // taken by tokenizing the option '--<option>=<arg1>'
        std::string arg = args[0].substr(found + 1);
        option.value.push_back(arg);
        option.original_tokens.push_back(arg);
    }
    // Discard the option at the front
    args.erase(args.begin());
}

template <typename PREDICATE>
void parse_positional_arguments(
    ClientOptionsParser::option_t& option,
    ClientOptionsParser::option_set_t& processed_options,
    ClientOptionsParser::arguments_set_t& args,
    size_t maximum_positional_args,
    PREDICATE predicate = [](const std::string&) { return true; }) {

    // Collect up to N positional arguments, that satisfy the predicate
    for (size_t i = 0; i < maximum_positional_args && !args.empty(); ++i) {
        // Take each of the positional values as an option value
        std::string& arg = args.front();
        // Once we find the first argument that does not satisfy the predicate, we stop collecting arguments
        if (!predicate(arg)) {
            break;
        }
        option.value.push_back(arg);
        option.original_tokens.push_back(arg);

        // Remove the argument value
        args.erase(args.begin());
    }
}

void parse_positional_arguments(ClientOptionsParser::option_t& option,
                                ClientOptionsParser::option_set_t& processed_options,
                                ClientOptionsParser::arguments_set_t& args,
                                size_t maximum_positional_args) {

    // Collect up to N positional arguments
    parse_positional_arguments(
        option, processed_options, args, maximum_positional_args, [](const std::string&) { return true; });
}

void parse_alter(ClientOptionsParser::option_set_t& processed_options, ClientOptionsParser::arguments_set_t& args) {

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

    ClientOptionsParser::option_t alter{std::string{"alter"}, {}};

    // This effectively always collects an argument (i.e. the operation)
    parse_option(alter, processed_options, args);

    // Collect up to 4 positional arguments, that are not paths
    parse_positional_arguments(
        alter, processed_options, args, 4, [](const std::string& arg) { return !is_valid_path(arg); });

    // Collect remaining positional arguments, that are paths
    parse_positional_arguments(
        alter, processed_options, args, std::numeric_limits<size_t>::max(), [](const std::string& arg) {
            return is_valid_path(arg);
        });

    processed_options.push_back(alter);
}

void parse_label(ClientOptionsParser::option_set_t& processed_options, ClientOptionsParser::arguments_set_t& args) {

    // *** Important! ***
    // This custom handler is needed to ensure that the "--label" option
    // special value parameters are handled correctly. For example,
    // values starting with -, such as "-j64".
    //
    // The custom handling will consider that 2 positional values (not
    // to be confused with positional arguments) are provided with the
    // --label option, as per one of the following forms:
    //     1) --label arg1 arg2
    //     2) --label=arg1 arg2

    ClientOptionsParser::option_t label{std::string{"label"}, {}};

    // This effectively always collects an argument (i.e. the label name)
    parse_option(label, processed_options, args);

    // Collect 1 positional argument (i.e. the label value)
    parse_positional_arguments(label, processed_options, args, 1);

    processed_options.push_back(label);
}

void parse_meter(ClientOptionsParser::option_set_t& processed_options, ClientOptionsParser::arguments_set_t& args) {

    // *** Important! ***
    // This custom handler is needed to ensure that the "--meter" option
    // special value parameters are handled correctly. For example,
    // values starting with -, such as "-1".
    //
    // The custom handling will consider that 2 positional values (not
    // to be confused with positional arguments) are provided with the
    // --label option, as per one of the following forms:
    //     1) --label arg1 arg2
    //     2) --label=arg1 arg2

    ClientOptionsParser::option_t meter{std::string{"meter"}, {}};

    // This effectively always collects an argument (i.e. the meter name)
    parse_option(meter, processed_options, args);

    // Collect 1 positional argument (i.e. the meter value)
    parse_positional_arguments(meter, processed_options, args, 1);

    processed_options.push_back(meter);
}

void parse_abort(ClientOptionsParser::option_set_t& processed_options, ClientOptionsParser::arguments_set_t& args) {

    // *** Important! ***
    // This custom handler is needed to ensure that the "--abort" option
    // special value parameters are handled correctly. For example,
    // values starting with -, such as "--some reason--".
    //
    // The custom handling will consider that 2 positional values (not
    // to be confused with positional arguments) are provided with the
    // --label option, as per one of the following forms:
    //     1) --label arg1 arg2
    //     2) --label=arg1 arg2

    ClientOptionsParser::option_t abort{std::string{"abort"}, {}};

    // This effectively always collects an argument (i.e. the reason text)
    parse_option(abort, processed_options, args);

    processed_options.push_back(abort);
}

} // namespace

ClientOptionsParser::option_set_t ClientOptionsParser::operator()(ClientOptionsParser::arguments_set_t& args) {
    option_set_t processed_options;

    if (ecf::algorithm::starts_with(args[0], "--alter")) {
        parse_alter(processed_options, args);
    }
    else if (ecf::algorithm::starts_with(args[0], "--label")) {
        parse_label(processed_options, args);
    }
    else if (ecf::algorithm::starts_with(args[0], "--meter")) {
        parse_meter(processed_options, args);
    }
    else if (ecf::algorithm::starts_with(args[0], "--abort")) {
        parse_abort(processed_options, args);
    }
    return processed_options;
}

} // namespace ecf
