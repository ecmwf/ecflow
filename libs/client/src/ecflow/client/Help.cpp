/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/client/Help.hpp"

#include <iomanip>
#include <optional>

#include "ecflow/base/HelpCatalog.hpp"
#include "ecflow/core/Child.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"

namespace /* __anonymous__ */ {

struct CommandFilter
{
    template <typename PREDICATE>
    static void select_by(Help::descriptions_t& options, PREDICATE select) {
        // filter non-command options
        Help::descriptions_t filtered;
        std::copy_if(std::begin(options),
                     std::end(options),
                     std::back_inserter(filtered),
                     [&select](const auto& description) { return select(description->long_name()); });

        // consider only filtered options
        std::swap(options, filtered);
    }

    static bool is_option(const std::string& value) {
        return std::find(std::begin(known_options), std::end(known_options), value) != std::end(known_options);
    }

    static bool is_user_command(const std::string& value) { return !is_option(value) && !is_task_command(value); }

    static bool is_task_command(const std::string& value) { return ecf::Child::valid_child_cmd(value); }

    static bool is_command(const std::string& value) { return is_task_command(value) || is_user_command(value); }

private:
    constexpr static std::array
        known_options{"add", "debug", "host", "password", "port", "rid", "ssl", "user", "http", "https"};
};

///
/// @brief Formats a single environment-variable manifest entry as two indented help lines.
///
/// @details The first line carries the variable name, its type, and its requirement level; a
/// trailing asterisk marks a variable whose value can be overridden by a command-line option. The
/// second line carries the description. The layout matches the documentation renderer in
/// docs/client_api/build.py, so that the CLI help and the generated documentation agree.
///
/// @param[in] var Manifest entry taken from the "environment_variables" array; must contain the
///                "required", "name", "type", and "description" fields, and may contain
///                "overridable_by".
/// @return The formatted two-line block, terminated by a newline.
///
std::string format_env_var(const nlohmann::json& var) {
    std::string required = var.at("required").get<std::string>();
    if (var.contains("overridable_by")) {
        required += "*";
    }

    std::string line = "  ";
    line += var.at("name").get<std::string>();
    line += " <";
    line += var.at("type").get<std::string>();
    line += "> [";
    line += required;
    line += "]\n    ";
    line += var.at("description").get<std::string>();
    line += "\n";
    return line;
}

///
/// @brief Reports whether an environment variable applies to the current build.
///
/// @details A variable may carry a "requires" field naming the build feature that gates it (for
/// example, ECF_SSL requires ECF_OPENSSL). A gated variable is advertised only when its feature is
/// compiled in, so that a client built without a feature does not describe variables that have no
/// effect. Variables with no "requires" field, or requiring a feature that is present, are enabled.
///
/// @param[in] var Manifest entry taken from the "environment_variables" array.
/// @return true when the variable applies to the current build, false when a required feature is absent.
///
bool env_var_enabled([[maybe_unused]] const nlohmann::json& var) {
#ifndef ECF_OPENSSL
    if (var.contains("requires") && var.at("requires") == "ECF_OPENSSL") {
        return false;
    }
#endif
    return true;
}

///
/// @brief Builds the environment-variable help common to both user and task commands.
///
/// @details Collects every manifest entry whose "applies_to" field is "both" and appends a note
/// explaining the asterisk convention used for values that a command-line option can override.
///
/// @return The assembled help text.
///
std::string make_client_env_description() {
    std::string help;
    help += "The client considers, for both user and task commands, the following environment variables:\n\n";

    for (const auto& var : ecf::HelpCatalog::manifest().at("environment_variables")) {
        if (var.at("applies_to").get<std::string>() == "both" && env_var_enabled(var)) {
            help += format_env_var(var);
        }
    }

    help += "\nThe options marked with (*) must be specified in order for the client to communicate\n"
            "with the server, either by setting the environment variables or by specifying the\n"
            "command line options.\n";

    return help;
}

///
/// @brief Builds the environment-variable help specific to task commands.
///
/// @details Collects every manifest entry whose "applies_to" field is "task" and appends a note
/// reminding that the mandatory variables are expected to be exported by the scripts.
///
/// @return The assembled help text.
///
std::string make_task_env_description() {
    std::string help;
    help += "The following environment variables are used specifically by task commands:\n\n";

    for (const auto& var : ecf::HelpCatalog::manifest().at("environment_variables")) {
        if (var.at("applies_to").get<std::string>() == "task" && env_var_enabled(var)) {
            help += format_env_var(var);
        }
    }

    help += "\nThe scripts are expected to export the mandatory variables, typically in shared include files\n";

    return help;
}

///
/// @brief Returns the environment-variable help common to user and task commands.
///
/// @details Built once on first use and cached, mirroring HelpCatalog::manifest()'s own lazy
/// parse: ecflow_client is invoked by a task potentially thousands of times over a suite run, and
/// only a small fraction of those invocations ever request help, so this must not force a JSON
/// parse on every one.
///
/// @return Reference to the cached text, valid for the remaining lifetime of the process.
///
const std::string& client_env_description() {
    static const std::string instance = make_client_env_description();
    return instance;
}

///
/// @brief Returns the environment-variable help specific to task commands.
///
/// @details Built once on first use and cached, for the same reason as client_env_description().
///
/// @return Reference to the cached text, valid for the remaining lifetime of the process.
///
const std::string& client_task_env_description() {
    static const std::string instance = make_task_env_description();
    return instance;
}

///
/// @brief Returns every definition-item name from the manifest, sorted alphabetically.
///
/// @details Definition items (node types and attributes documented in
/// docs/ug/user_manual/definition_file_format.rst) are not Boost-registered options, so they are
/// read directly from ecf::HelpCatalog::manifest() rather than from a Help::descriptions_t.
///
/// @return The sorted list of names.
///
std::vector<std::string> sorted_definition_item_names() {
    std::vector<std::string> names;
    for (const auto& item : ecf::HelpCatalog::manifest().at("definitions")) {
        names.push_back(item.at("name").get<std::string>());
    }
    std::sort(names.begin(), names.end());
    return names;
}

///
/// @brief Returns a fixed-width label for a definition-item's kind, for column-aligned listings.
///
/// @param[in] item Manifest entry taken from the "definitions" array; must contain "kind".
/// @return "node     " or "attribute", both nine characters wide.
///
std::string get_definition_item_kind(const nlohmann::json& item) {
    return item.at("kind").get<std::string>() == "node" ? "node     " : "attribute";
}

int get_options_max_width(const Help::descriptions_t& options) {
    size_t vec_size  = options.size();
    size_t max_width = 0;
    for (size_t i = 0; i < vec_size; i++) {
        max_width = std::max(max_width, options[i]->long_name().size());
    }
    max_width += 1;
    return static_cast<int>(max_width);
}

void sort_options_by_long_name(Help::descriptions_t& options) {
    std::sort(
        options.begin(), options.end(), [](const auto& a, const auto& b) { return a->long_name() < b->long_name(); });
}

class Documentation {
public:
    using descriptions_t = boost::program_options::options_description;

    explicit Documentation(const descriptions_t& descriptions)
        : descriptions_{descriptions} {}

    void show(std::ostream& os, const std::string& topic) const;

private:
    void show_help(std::ostream& os) const;
    void show_list_options(std::ostream& os) const;
    void show_all_commands_summary(std::ostream& os, std::string_view title) const;
    void show_task_commands_summary(std::ostream& os, std::string_view title) const;
    void show_user_commands_summary(std::ostream& os, std::string_view title) const;
    void show_options_summary(std::ostream& os, std::string_view title) const;
    void show_command_help(std::ostream& os, const std::string& command) const;
    void show_all_commands(std::ostream& os, std::string_view title) const;
    void show_all_options(std::ostream& os) const;
    void show_definition_items_summary(std::ostream& os, std::string_view title) const;
    void show_definition_item_help(std::ostream& os, const std::string& name) const;
    void show_all_definitions(std::ostream& os, std::string_view title) const;

    template <typename PREDICATE>
    void show_table(std::ostream& os, PREDICATE select, size_t columns) const;

    template <typename PREDICATE>
    void show_summary(std::ostream& os, PREDICATE select) const;

    static std::string get_name_kind(const std::string& name) {
        if (CommandFilter::is_option(name)) {
            return "option  ";
        }
        else if (CommandFilter::is_task_command(name)) {
            return "task    ";
        }
        else if (CommandFilter::is_user_command(name)) {
            return "user    ";
        }
        else {
            throw std::runtime_error("Unable to determine the kind of option/command");
        }
    }

private:
    const descriptions_t& descriptions_;
};

void Documentation::show(std::ostream& os, const std::string& topic) const {
    // WARNING!!
    //   This assumes that there are no user/task commands named:
    //   'summary', 'all', 'task', 'user', 'option', 'definition'
    //

    if (topic.empty()) {
        show_help(os);
    }
    else if (topic == "all") {
        show_list_options(os);
    }
    else if (topic == "summary") {
        show_all_commands_summary(os, "\nEcflow client commands:\n");
    }
    else if (topic == "task") {
        show_task_commands_summary(os, "\nEcflow task client commands:\n");
    }
    else if (topic == "user") {
        show_user_commands_summary(os, "\nEcflow user client commands:\n");
    }
    else if (topic == "option") {
        show_options_summary(os, "\nEcflow generic options:\n");
    }
    else if (topic == "definition") {
        show_definition_items_summary(os, "\nEcflow definition items:\n");
    }
    else if (topic.rfind("defs/", 0) == 0) {
        // A dedicated prefix, rather than a bare name, so that a definition-file item whose name
        // collides with an existing command (e.g. the "event" attribute vs. the --event command)
        // is always unambiguous; bare command/option/topic lookups above are unaffected.
        show_definition_item_help(os, topic.substr(5));
    }
    else {
        show_command_help(os, topic);
    }
}

void Documentation::show_help(std::ostream& os) const {
    os << "\nClient/server based work flow package:\n\n";
    os << ecf::Version::description() << "\n\n";
    os << Ecf::CLIENT_NAME() << " provides the command line interface, for interacting with the server:\n";

    os << "Try:\n\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=all         # List all commands, verbosely\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=summary     # One line summary of all commands\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=task        # One line summary of task commands\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=user        # One line summary of user command\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=<command>   # Detailed help on a specific <command>\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=definition  # List all definition-file items\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=defs/<item> # Detailed help on a specific definition <item>\n\n";

    show_all_commands(os, "Commands:");

    show_all_options(os);

    show_all_definitions(os, "Definition:");
}

void Documentation::show_list_options(std::ostream& os) const {
    // The options are shown using a very plain list of all the options, with their description.

    os << "\n  Client options:\n\n    " << ecf::Version::description() << "\n\n";

    for (const auto& option : descriptions_.options()) {
        std::string description;
        if (std::optional<std::string> text = ecf::HelpCatalog::description_for(option->long_name())) {
            description = *text;
        }
        else {
            description = "No description was found for this option.\n"
                          "Please report this issue to the development team.";
        }

        os << "    --" << option->long_name() << "\n\n";

        std::vector<std::string> lines;
        ecf::algorithm::split_fields_at(lines, description, "\n");
        for (const auto& line : lines) {
            if (!line.empty()) {
                os << "        " << line;
            }
            os << "\n";
        }
        os << "\n";
    }
}

template <typename PREDICATE>
void Documentation::show_table(std::ostream& os, PREDICATE select, size_t columns) const {
    // take a copy, since we need to sort
    auto options = descriptions_.options();

    // filter for real commands
    CommandFilter::select_by(options, select);

    // sort using long_name
    sort_options_by_long_name(options);

    size_t max_width = get_options_max_width(options);
    for (size_t i = 0; i < options.size(); i++) {
        if (i == 0 || i % columns == 0) {
            os << "\n   ";
        }
        os << std::left << std::setw(max_width) << options[i]->long_name();
    }
    os << "\n\n";
}

template <typename PREDICATE>
void Documentation::show_summary(std::ostream& os, PREDICATE select) const {

    // take a copy, since we need to sort
    auto options = descriptions_.options();

    // filter for real commands
    CommandFilter::select_by(options, select);

    // sort using long_name
    sort_options_by_long_name(options);

    int max_width = get_options_max_width(options);
    for (const auto& option : options) {
        std::string name = option->long_name();

        std::string first_line;
        if (std::optional<std::string> summary = ecf::HelpCatalog::summary_for(name)) {
            first_line = *summary;
        }
        else {
            first_line = ecf::HelpCatalog::not_provided;
        }

        os << "  " << std::left << std::setw(max_width) << name << " ";
        os << Documentation::get_name_kind(name);
        os << first_line << "\n";
    }
    os << "\n";
}

void Documentation::show_all_commands_summary(std::ostream& os, std::string_view title) const {
    os << title << '\n';
    show_summary(os, CommandFilter::is_command);
}

void Documentation::show_task_commands_summary(std::ostream& os, std::string_view title) const {
    os << title << '\n';
    show_summary(os, CommandFilter::is_task_command);
}

void Documentation::show_user_commands_summary(std::ostream& os, std::string_view title) const {
    os << title << '\n';
    show_summary(os, CommandFilter::is_user_command);
}

void Documentation::show_options_summary(std::ostream& os, std::string_view title) const {
    os << title << '\n';
    show_summary(os, CommandFilter::is_option);
}

void Documentation::show_command_help(std::ostream& os, const std::string& command) const {
    // Help on individual command
    const boost::program_options::option_description* od =
        descriptions_.find_nothrow(command,
                                   true,  /* approx, will find nearest match */
                                   false, /* long_ignore_case = false*/
                                   false  /* short_ignore_case = false*/
        );
    if (od) {
        os << "\n";
        os << od->long_name() << "\n";
        for (size_t i = 0; i < od->long_name().size(); i++) {
            os << "-";
        }
        os << "\n\n";
        if (std::optional<std::string> text = ecf::HelpCatalog::description_for(od->long_name())) {
            os << *text << "\n\n";
        }
        else {
            os << ecf::HelpCatalog::not_provided << "\n\n";
        }
        if (!CommandFilter::is_option(od->long_name())) {
            os << client_env_description();
            if (ecf::Child::valid_child_cmd(od->long_name())) {
                os << "\n";
                os << client_task_env_description();
            }
        }
    }
    else {
        show_all_commands(os, "No matching command found, please choose from:");
    }
}

void Documentation::show_all_commands(std::ostream& os, std::string_view title) const {
    os << title << "\n";
    show_table(os, CommandFilter::is_command, 5);
}

void Documentation::show_all_options(std::ostream& os) const {
    os << "Options:\n";
    show_table(os, CommandFilter::is_option, 8);
}

void Documentation::show_definition_items_summary(std::ostream& os, std::string_view title) const {
    os << title << '\n';

    std::vector<std::string> names = sorted_definition_item_names();

    size_t max_width = 0;
    for (const auto& name : names) {
        max_width = std::max(max_width, name.size());
    }
    max_width += 1;

    for (const auto& name : names) {
        const nlohmann::json* item = ecf::HelpCatalog::find_definition_item(name);
        std::string first_line =
            ecf::HelpCatalog::summary_for_definition_item(name).value_or(ecf::HelpCatalog::not_provided);

        os << "  " << std::left << std::setw(static_cast<int>(max_width)) << name << " ";
        os << get_definition_item_kind(*item) << " ";
        os << first_line << "\n";
    }
    os << "\n";
}

void Documentation::show_definition_item_help(std::ostream& os, const std::string& name) const {
    if (ecf::HelpCatalog::find_definition_item(name)) {
        os << "\n";
        os << name << "\n";
        for (size_t i = 0; i < name.size(); i++) {
            os << "-";
        }
        os << "\n\n";
        os << ecf::HelpCatalog::description_for_definition_item(name).value_or(ecf::HelpCatalog::not_provided);
        os << "\n\n";
    }
    else {
        // Same-category fallback: an unknown "defs/<name>" lists the valid definition items,
        // not the command list, since that is what the user was actually asking about.
        show_definition_items_summary(os, "No matching definition item found, please choose from:");
    }
}

void Documentation::show_all_definitions(std::ostream& os, std::string_view title) const {
    os << title << "\n";

    std::vector<std::string> names = sorted_definition_item_names();

    size_t max_width = 0;
    for (const auto& name : names) {
        max_width = std::max(max_width, name.size());
    }
    max_width += 1;

    for (size_t i = 0; i < names.size(); i++) {
        if (i == 0 || i % 5 == 0) {
            os << "\n   ";
        }
        os << std::left << std::setw(static_cast<int>(max_width)) << names[i];
    }
    os << "\n\n";
}

} // namespace

struct Help::Impl
{
    Documentation documentation_;
    std::string topic_;

    Impl(const description_t& description, std::string topic)
        : documentation_(description),
          topic_(std::move(topic)) {}
};

Help::Help(const description_t& description, const std::string& topic)
    : impl_(std::make_unique<Help::Impl>(description, topic)) {
}

Help::~Help() = default;

std::ostream& operator<<(std::ostream& os, const Help& help) {
    help.impl_->documentation_.show(os, help.impl_->topic_);
    return os;
}
