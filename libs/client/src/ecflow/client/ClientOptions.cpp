/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

///
/// \brief Delegates argument parsing to the registered commands
///

#include "ecflow/client/ClientOptions.hpp"

#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>

#include "ecflow/base/ClientOptionsParser.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/Help.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"

using namespace std;
using namespace ecf;
using namespace boost;
namespace po = boost::program_options;

static std::string print_variable_map(const boost::program_options::variables_map& vm);

ClientOptions::ClientOptions() {
    // This could have been moved to parse(). However, since the same ClientInvoker can be
    // used for multiple commands, we have separated out the parts.
    // As this needs to be done only once, this approach improves the performance.
    std::string title_help = "Client options, ";
    title_help += Version::description();
    title_help += "   ";
    desc_ = new po::options_description(title_help, po::options_description::m_default_line_length + 80);

    // This will iterate over all the registered client to server commands and
    // Each command will add to the option description, its required arguments
    cmdRegistry_.addAllOptions(*desc_);

    // Allow the host,port and rid to be  overridden by the command line
    // This allows the jobs, which make other calls to ecflow_client from interfering with each other
    // clang-format off
    desc_->add_options()(
        "rid",
        po::value<string>()->implicit_value(string("")),
        "When specified overrides the environment variable ECF_RID. Can only be used for child commands.");
    desc_->add_options()(
        "port",
        po::value<string>()->implicit_value(string("")),
        "When specified overrides the environment variable ECF_PORT and default port: '3141'");
    desc_->add_options()(
        "host",
        po::value<string>()->implicit_value(string("")),
        "When specified overrides the environment variable ECF_HOST and default host: 'localhost'");
    desc_->add_options()(
        "user",
        po::value<string>()->implicit_value(string("")),
        "Specifies the user name used to contact the server. Must be used in combination with option --password.");
    desc_->add_options()(
        "password",
        po::value<string>()->implicit_value(string("")),
        "Specifies the password used to contact the server. Must be used in combination with option --user.");
#ifdef ECF_OPENSSL
    desc_->add_options()(
        "ssl",
        "Enables the use of SSL when contacting the server.\n"
        "When specified overrides the environment variable ECF_SSL.");
#endif
    desc_->add_options()(
        "http",
        "Enables communication over HTTP between client/server.\n");
    desc_->add_options()(
        "https",
        "Enables communication over HTTPS between client/server.\n");
    // clang-format on
}

ClientOptions::~ClientOptions() {
    delete desc_;
}

Cmd_ptr ClientOptions::parse(const CommandLine& cl, ClientEnvironment* env) const {
    // We expect two hyphen/minus, However sometimes we get a weird/rogue kind of hyphen
    // This rogue hyphen can mess up the parsing.
    // # ecflow_client ––group="halt=yes; check_pt; terminate=yes"  // *BAD* hyphens 2 of them
    // # ecflow_client –-group="halt=yes; check_pt; terminate=yes"  // *BAD* hyphens 1 of them, i.e. first
    // # ecflow_client --group="halt=yes; check_pt; terminate=yes"  // *GOOD*
    //
    //   dec:  -30 ffffffe2 37777777742 \342
    //   hex: -128 ffffff80 37777777600 \200
    //   oct: -109 ffffff93 37777777623 \223
    //
    // The correct hyphen has:
    //   dec:45 hex:2D oct:55 -
    if (env->debug()) {
        cout << "  ClientOptions::parse " << cl << "\n";
        std::cout << "  help column width = " << po::options_description::m_default_line_length + 80 << "\n";
    }

    // parse arguments into 'vm'.
    //       --alter delete cron -w 0,1 10:00 /s1     # -w treated as option
    //       --alter=/s1 change meter name -1         # -1 treated as option
    // Note: negative numbers get treated as options: i.e. trying to change meter value to a negative number
    //       To avoid negative numbers from being treated as option use, we need to change command line style:
    //       po::command_line_style::unix_style ^ po::command_line_style::allow_short
    boost::program_options::variables_map vm;

    // 1) Parse the CLI options
    po::parsed_options parsed_options = po::command_line_parser(cl.tokens())
                                            .options(*desc_)
                                            .style(po::command_line_style::default_style)
                                            .extra_style_parser(ClientOptionsParser{})
                                            .run();

    // 2) Store the CLI options into the variable map
    po::store(parsed_options, vm);
    po::notify(vm);

    // If explicitly requested by user, set environment in DEBUG mode
    if (vm.count("debug")) {
        env->set_debug(true);
    }

    // Check to see if host or port, specified. This will override the environment variables
    std::string host, port;
    if (vm.count("port")) {
        port = vm["port"].as<std::string>();
        if (env->debug())
            std::cout << "  port " << port << " overridden at the command line\n";
        try {
            ecf::convert_to<int>(port);
        }
        catch (ecf::bad_conversion& e) {
            std::stringstream ss;
            ss << "ClientOptions::parse: The specified port(" << port << ") must be convertible to an integer";
            throw std::runtime_error(ss.str());
        }
    }
    if (vm.count("host")) {
        host = vm["host"].as<std::string>();
        if (env->debug())
            std::cout << "  host " << host << " overridden at the command line\n";
    }
    if (!host.empty() || !port.empty()) {
        if (host.empty())
            host = env->hostSpecified(); // get the environment variable ECF_HOST
        if (port.empty())
            port = env->portSpecified(); // get the environment variable ECF_PORT || Str::DEFAULT_PORT_NUMBER()
        if (host.empty())
            host = Str::LOCALHOST(); // if ECF_HOST not specified default to localhost
        if (port.empty())
            port = Str::DEFAULT_PORT_NUMBER(); // if ECF_PORT not specified use default
        env->set_host_port(host, port);
    }
    if (vm.count("rid")) {
        std::string rid = vm["rid"].as<std::string>();
        if (env->debug())
            std::cout << "  rid " << rid << " overridden at the command line\n";
        env->set_remote_id(rid);
    }
    if (vm.count("user")) {
        std::string user = vm["user"].as<std::string>();
        if (env->debug())
            std::cout << "  user " << user << " overridden at the command line\n";
        env->set_user_name(user);
    }
    if (vm.count("password")) {
        std::string password = vm["password"].as<std::string>();
        if (env->debug())
            std::cout << "  password overridden at the command line\n";
        env->set_password(PasswordEncryption::encrypt(password, env->get_user_name()));
    }

#ifdef ECF_OPENSSL
    if (auto ecf_ssl = getenv("ECF_SSL"); vm.count("ssl") || ecf_ssl) {
        if (!vm.count("ssl") && ecf_ssl) {
            if (env->debug()) {
                std::cout << "  ssl enabled via environment variable\n";
            }
            env->enable_ssl_if_defined();
        }
        else if (vm.count("ssl") && !ecf_ssl) {
            if (env->debug()) {
                std::cout << "  ssl explicitly enabled via command line\n";
            }
            env->enable_ssl();
        }
        else {
            if (env->debug()) {
                std::cout << "  ssl explicitly enabled via command line, but also enabled via environment variable\n";
            }
            env->enable_ssl_if_defined();
        }

        if (env->debug()) {
            std::cout << "  ssl certificate: '" << env->openssl().info() << "' \n";
        }
    }
#endif

    if (vm.count("http")) {
        if (env->debug())
            std::cout << "  http set via command line\n";
        env->enable_http();
    }
    if (vm.count("https")) {
        if (env->debug())
            std::cout << "  https set via command line\n";
        env->enable_https();
    }

    // Defer the parsing of the command , to the command. This allows
    // all cmd functionality to be centralised with the command
    // This can throw std::runtime_error if the arguments do not parse
    Cmd_ptr client_request;
    if (!cmdRegistry_.parse(client_request, vm, env)) {

        // The arguments did *NOT* match with any of the registered command.
        // Hence, if arguments don't match help, debug or version it's an error
        // Note: we did *NOT* check for a NULL client_request since *NOT* all
        //       requests need to create it. Some commands are client specific.
        //       For example:
        //         --server_load         // this is sent to server
        //         --server_load=<path>  // no command returned, command executed by client
        if (vm.count("help")) {
            string topic = vm["help"].as<std::string>();
            std::cout << Help{*desc_, topic};
            return client_request;
        }

        if (vm.count("version")) {
            cout << Version::description() << "\n";
            exit(0);
        }

        std::stringstream ss;
        ss << print_variable_map(vm) << "\n";
        ss << "ClientOptions::parse: Arguments did not match any commands.\n";
        ss << "  argc=" << cl.size() << "\n";
        for (size_t i = 0; i < cl.size(); i++) {
            const std::string& arg = cl.tokens()[i];
            ss << "  arg" << i << "=" << arg;

            for (size_t s = 0; s < arg.size(); s++) {
                if (static_cast<int>(arg[s]) < 0) {
                    ss << "\nUnrecognised character not in ASCII range(0-127) " << std::dec << "dec("
                       << static_cast<int>(arg[s]) << ") char:" << arg[s];
                    ss << " found at index " << s << " for string '" << arg << "'\n";
                    if (static_cast<int>(arg[s]) == -30)
                        ss << "check for bad hyphen/minus";
                    throw std::runtime_error(ss.str());
                }
            }
        }
        ss << "\nUse --help to see all the available commands\n";
        throw std::runtime_error(ss.str());
    }

    return client_request;
}

static std::string print_variable_map(const boost::program_options::variables_map& vm) {
    std::stringstream ss;
    ss << "boost::program_options::variables_map:    vm.size() " << vm.size() << "\n";
    for (po::variables_map::const_iterator it = vm.begin(); it != vm.end(); it++) {
        std::cout << "> " << it->first;
        if (((boost::any)it->second.value()).empty())
            ss << "(empty)";
        if (vm[it->first].defaulted() || it->second.defaulted())
            ss << "(default)";

        ss << "=";

        bool is_char;
        try {
            boost::any_cast<const char*>(it->second.value());
            is_char = true;
        }
        catch (const boost::bad_any_cast&) {
            is_char = false;
        }
        bool is_str;
        try {
            boost::any_cast<std::string>(it->second.value());
            is_str = true;
        }
        catch (const boost::bad_any_cast&) {
            is_str = false;
        }

        if (((boost::any)it->second.value()).type() == typeid(int)) {
            ss << vm[it->first].as<int>() << std::endl;
        }
        else if (((boost::any)it->second.value()).type() == typeid(bool)) {
            ss << vm[it->first].as<bool>() << std::endl;
        }
        else if (((boost::any)it->second.value()).type() == typeid(double)) {
            ss << vm[it->first].as<double>() << std::endl;
        }
        else if (is_char) {
            ss << vm[it->first].as<const char*>() << std::endl;
        }
        else if (is_str) {
            std::string temp = vm[it->first].as<std::string>();
            if (temp.size()) {
                ss << temp << std::endl;
            }
            else {
                ss << "true" << std::endl;
            }
        }
        else { // Assumes that the only remainder is vector<string>
            try {
                std::vector<std::string> vect = vm[it->first].as<std::vector<std::string>>();
                size_t i                      = 0;
                for (std::vector<std::string>::iterator oit = vect.begin(); oit != vect.end(); oit++, ++i) {
                    ss << "\r> " << it->first << "[" << i << "]=" << (*oit) << std::endl;
                }
            }
            catch (const boost::bad_any_cast&) {
                ss << "UnknownType(" << ((boost::any)it->second.value()).type().name() << ")" << std::endl;
            }
        }
    }
    return ss.str();
}
