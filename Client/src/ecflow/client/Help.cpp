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

#include "ecflow/core/Child.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"

namespace /* __anonymous__ */ {

using description_t = boost::program_options::option_description;
using options_t     = std::vector<boost::shared_ptr<boost::program_options::option_description>>;

struct CommandFilter
{
    template <typename PREDICATE>
    static void select_by(options_t& options, PREDICATE select) {
        // filter non-command options
        options_t filtered;
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
    constexpr static std::array known_options{"add", "debug", "host", "password", "port", "rid", "ssl", "user"};
};

const char* client_env_description =
    "The client reads in the following environment variables. These are read by user and child command\n\n"
    "|----------|----------|------------|-------------------------------------------------------------------|\n"
    "| Name     |  Type    | Required   | Description                                                       |\n"
    "|----------|----------|------------|-------------------------------------------------------------------|\n"
    "| ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |\n"
    "| ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |\n"
#ifdef ECF_OPENSSL
    "| ECF_SSL  |  <any>   | Optional*  | Enable encrypted comms with SSL enabled server.                   |\n"
#endif
    "|----------|----------|------------|-------------------------------------------------------------------|"
    "\n\n"
    "* The host and port must be specified in order for the client to communicate with the server, this can \n"
    "  be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line\n";

const char* client_task_env_description =
    "The following environment variables are specific to child commands.\n"
    "The scripts should export the mandatory variables. Typically defined in the head/tail includes files\n\n"
    "|--------------|----------|-----------|---------------------------------------------------------------|\n"
    "| Name         |  Type    | Required  | Description                                                   |\n"
    "|--------------|----------|-----------|---------------------------------------------------------------|\n"
    "| ECF_NAME     | <string> | Mandatory | Full path name to the task                                    |\n"
    "| ECF_PASS     | <string> | Mandatory | The jobs password, allocated by server, then used by server to|\n"
    "|              |          |           | authenticate client request                                   |\n"
    "| ECF_TRYNO    |  <int>   | Mandatory | The number of times the job has run. This is allocated by the |\n"
    "|              |          |           | server, and used in job/output file name generation.          |\n"
    "| ECF_RID      | <string> | Mandatory | The process identifier. Helps zombies identification and      |\n"
    "|              |          |           | automated killing of running jobs                             |\n"
    "| ECF_TIMEOUT  |  <int>   | optional  | Max time in *seconds* for client to deliver message to main   |\n"
    "|              |          |           | server. The default is 24 hours                               |\n"
    "| ECF_HOSTFILE | <string> | optional  | File that lists alternate hosts to try, if connection to main |\n"
    "|              |          |           | host fails                                                    |\n"
    "| ECF_DENIED   |  <any>   | optional  | Provides a way for child to exit with an error, if server     |\n"
    "|              |          |           | denies connection. Avoids 24hr wait. Note: when you have      |\n"
    "|              |          |           | hundreds of tasks, using this approach requires a lot of      |\n"
    "|              |          |           | manual intervention to determine job status                   |\n"
    "| NO_ECF       |  <any>   | optional  | If set exit's ecflow_client immediately with success. This    |\n"
    "|              |          |           | allows the scripts to be tested independent of the server     |\n"
    "|--------------|----------|-----------|---------------------------------------------------------------|\n";

int get_options_max_width(const options_t& options) {
    size_t vec_size  = options.size();
    size_t max_width = 0;
    for (size_t i = 0; i < vec_size; i++) {
        max_width = std::max(max_width, options[i]->long_name().size());
    }
    max_width += 1;
    return static_cast<int>(max_width);
}

void sort_options_by_long_name(options_t& options) {
    std::sort(
        options.begin(), options.end(), [](const auto& a, const auto& b) { return a->long_name() < b->long_name(); });
}

class Documentation {
public:
    using descriptions_t = boost::program_options::options_description;

    explicit Documentation(const descriptions_t& descriptions) : descriptions_{descriptions} {}

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

    template <typename PREDICATE>
    void show_table(std::ostream& os, PREDICATE select, size_t columns) const;

    template <typename PREDICATE>
    void show_summary(std::ostream& os, PREDICATE select) const;

    static std::string get_name_kind(const std::string& name) {
        if (CommandFilter::is_option(name)) {
            return "option  ";
        }
        else if (CommandFilter::is_task_command(name)) {
            return "child   ";
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
    //   This assumes that there are no user/child commands named: 'summary', 'all', 'child', 'user'
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
    else if (topic == "child") {
        show_task_commands_summary(os, "\nEcflow child client commands:\n");
    }
    else if (topic == "user") {
        show_user_commands_summary(os, "\nEcflow user client commands:\n");
    }
    else if (topic == "option") {
        show_options_summary(os, "\nEcflow generic options:\n");
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
    os << "   " << Ecf::CLIENT_NAME() << " --help=all       # List all commands, verbosely\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=summary   # One line summary of all commands\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=child     # One line summary of child commands\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=user      # One line summary of user command\n";
    os << "   " << Ecf::CLIENT_NAME() << " --help=<cmd>     # Detailed help on each command\n\n";

    show_all_commands(os, "Commands:");

    show_all_options(os);
}

void Documentation::show_list_options(std::ostream& os) const {
    os << descriptions_ << "\n";
}

template <typename PREDICATE>
void Documentation::show_table(std::ostream& os, PREDICATE select, size_t columns) const {
    // take a copy, since we need to sort
    options_t options = descriptions_.options();

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
    options_t options = descriptions_.options();

    // filter for real commands
    CommandFilter::select_by(options, select);

    // sort using long_name
    sort_options_by_long_name(options);

    int max_width = get_options_max_width(options);
    for (const auto& option : options) {
        std::vector<std::string> lines;
        ecf::Str::split(option->description(), lines, "\n");
        if (!lines.empty()) {
            std::string name = option->long_name();
            os << "  " << std::left << std::setw(max_width) << name << " ";
            os << Documentation::get_name_kind(name);
            os << lines[0] << "\n";
        }
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
        os << od->description() << "\n\n";
        if (!CommandFilter::is_option(od->long_name())) {
            os << client_env_description;
            if (ecf::Child::valid_child_cmd(od->long_name())) {
                os << "\n";
                os << client_task_env_description;
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
    os << "Generic Options:\n";
    show_table(os, CommandFilter::is_option, 8);
}

} // namespace

struct Help::Impl
{
    Documentation documentation_;
    std::string topic_;

    Impl(const descriptions_t& descriptions, std::string topic)
        : documentation_(descriptions),
          topic_(std::move(topic)) {}
};

Help::Help(const descriptions_t& descriptions, const std::string& topic)
    : impl_(std::make_unique<Help::Impl>(descriptions, topic)) {
}

Help::~Help() = default;

std::ostream& operator<<(std::ostream& os, const Help& help) {
    help.impl_->documentation_.show(os, help.impl_->topic_);
    return os;
}
