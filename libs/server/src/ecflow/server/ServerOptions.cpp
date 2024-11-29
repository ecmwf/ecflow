/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/ServerOptions.hpp"

#include <iostream>

#include "ecflow/core/Version.hpp"
#include "ecflow/server/ServerEnvironment.hpp"

#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif

using namespace std;
using namespace ecf;
namespace po = boost::program_options;

ServerOptions::ServerOptions(const CommandLine& cl, ServerEnvironment* env) {
    std::stringstream ss;
    ss << "\n" << Version::description() << "\nServer options";
    po::options_description desc(ss.str(), po::options_description::m_default_line_length + 80);

    desc.add_options()("help,h",
                       "The server reads the LANG for the locale. Please ensure that\n"
                       "locale is valid otherwise the server will abort on startup.\n"
                       "For a list of valid locale use 'locale -a'.\n"
                       "\n"
                       "The server will read the following environment variables:\n"
                       "ECF_PORT:\n"
                       "  Defines the port that the server will listen to.\n"
                       "  The port number must be consistent between client and server\n"
                       "  If two servers are started on the same machine with same port\n"
                       "  then a 'Address in use' error is shown and server will exit.\n"
                       "  The default value for client/server is 3141\n"
                       "ECF_HOME:\n"
                       "  This is the home/root for all '.ecf' scripts.\n"
                       "  When the server starts it will change directory to ECF_HOME,\n"
                       "  hence the directory must be accessible and writable.\n"
                       "  The default value is the current working directory\n"
                       "ECF_LOG:\n"
                       "  Overrides the name of log file.\n"
                       "  The default log file name is: <host>.<port>.ecf.log, i.e. machine1.3141.ecf.log\n"
                       "  this is required since we can have multiple servers for a single machine.\n"
                       "  Where each server will have a separate port number.\n"
                       "  Note: Any settings will be prepended with <host>.<port>.\n"
                       "ECF_CHECK:\n"
                       "  Defines the name of the check point file.\n"
                       "  This stores the state of the definition in memory,\n"
                       "  and allows recovery from crash. By default the server on start up will load\n"
                       "  the check point file or back up check point file if they exist.\n"
                       "  The default is <host>.<port>.ecf.check\n"
                       "  Any settings will be prepended with <host>.<port>.\n"
                       "ECF_CHECKOLD:\n"
                       "  The name of the backup checkpoint file\n"
                       "  default is <host>.<port>.ecf.check.b\n"
                       "  Any settings will be prepended with <host>.<port>.\n"
                       "ECF_CHECKINTERVAL:\n"
                       "  The interval in seconds within the server that the checkpoint file is saved\n"
                       "  Values less than 60 seconds are not recommended\n"
                       "  The default value is 120 seconds\n"
                       "ECF_LISTS:\n"
                       "  This variable is used to identify a file, that lists the user\n"
                       "  who can access the server via client commands. Each client command\n"
                       "  (ignoring child commands, i.e init, complete, event, meter, label, wait)\n"
                       "  will encode the user name of the process initiating the client request\n"
                       "  This is then compared with list of users in the ecf.lists file.\n"
                       "  If this file is empty, then no authentication is done.\n"
                       "  Each server can potentially have a different list.\n"
                       "  The default is <host>.<port>.ecf.lists if no ECF_LISTS is specified.\n"
                       "  Note: If the path to a ecf.lists is specified as i.e /var/tmp/ecflow/ecf.lists\n"
                       "        then no prefix is added, the path is kept as is.\n"
                       "        However if 'ECF_LISTS=ecf.lists' then this matches the default, and the\n"
                       "        server will expect <host>.<port>.ecf.lists \n"
                       "ECF_TASK_THRESHOLD:\n"
                       "  The Job generation process is expected to take less than 60 seconds\n"
                       "  This is used to aid debugging of task taking excessive times for job generation\n"
                       "  The causes can be:\n"
                       "    a/ Slow disk I/0, or when server is run an a virtual machine\n"
                       "    b/ Insufficient memory, when compared to the size of the scripts/definition\n"
                       "    c/ Large number of jobs submitted at the same time, and/or very large scripts\n"
                       "  Any task taking longer than the specified time in milliseconds will be logged.\n"
                       "  WAR:[..] Job generation for task /suite/family/dodgy_task took 5000ms,\n"
                       "  Exceeds ECF_TASK_THRESHOLD. The default threshold is 4000 milliseconds\n"
                       "  Note: 1000 milliseconds = 1 second\n"
                       "    export ECF_TASK_THRESHOLD=1500\n"
                       "ECF_PRUNE_NODE_LOG:\n"
                       "  The node log history is stored in memory and written to the checkpoint file as backup.\n"
                       "  Overtime this can build up. If the server is restored from a checkpoint file, then all\n"
                       "  node log/edit history older than 30 days is automatically pruned, thus saving space\n"
                       "  in memory and disk. The environment variable of this name can be used to alter the days.\n"
                       "  If set to zero all edit history is preserved.\n"
#ifdef ECF_OPENSSL
                       "ECF_SSL:\n"
                       "  Enables encrypted communication between client and server.\n"
                       "  You will need to ensure open ssl in installed on your system\n"
                       "  In order to use openssl, we need set up some certificates.\n"
                       "  (These will be self signed certificates, rather than a certificate authority).\n"
#endif
                       "\n"
                       "These defaults along with several other can be specified in the file\n"
                       "server_environment.cfg. The file should be placed in the current working\n"
                       "directory. It can be found in ecFlow source tree under Server/ directory.\n"
                       "Please read this file to see the available options.\n"
                       "Note: Environment variables _override_ any settings made in the\n"
                       "server_environment.cfg file.")(
        "port",
        po::value<int>(),
        "<int> <Allowed range 1024-49151> The socket/port the server is listening too. default = 3141\n"
        "If set will override the environment variable ECF_PORT")(
        "ecfinterval", po::value<int>(), "<int> <Allowed range 1-60>  Submit jobs interval. For DEBUG/Test only")(
        "v6", "Use IPv6 TCP protocol. Default is IPv4")
#ifdef ECF_OPENSSL
        ("ssl", ecf::Openssl::ssl_info())
#endif
            ("dis_job_gen", "Disable job generation. For DEBUG/Test only.")("debug,d", "Enable debug output.")(
                "version,v",
                "Show ecflow version number,boost library version, compiler used and compilation date, then exit");

    // 1) Parse the CLI options
    po::parsed_options parsed_options = po::command_line_parser(cl.tokens())
                                            .options(desc)
                                            .style(po::command_line_style::default_style)
                                            .run();

    // 2) Store the CLI options into the variable map
    po::store(parsed_options, vm_);
    po::notify(vm_);

    if (vm_.count("help"))
        cout << desc << "\n";

    if (vm_.count("version")) {
        cout << Version::description() << "\n";
    }

    if (vm_.count("debug"))
        env->debug_ = true;

    if (vm_.count("port")) {
        if (env->debug_)
            cout << "ServerOptions:: The port number set to '" << vm_["port"].as<int>() << "'\n";
        env->serverPort_ = vm_["port"].as<int>();
    }
    if (vm_.count("ecfinterval")) {
        if (env->debug_)
            cout << "ServerOptions: The ecfinterval set to '" << vm_["ecfinterval"].as<int>() << "'\n";
        env->submitJobsInterval_ = vm_["ecfinterval"].as<int>();
    }
    if (vm_.count("v6")) {
        if (env->debug_)
            cout << "ServerOptions: The tcp protocol set to v6\n";
        env->tcp_protocol_ = boost::asio::ip::tcp::v6();
    }
    if (vm_.count("dis_job_gen")) {
        if (env->debug_)
            cout << "ServerOptions: The dis_job_gen is set\n";
        env->jobGeneration_ = false;
    }
#ifdef ECF_OPENSSL
    if (vm_.count("ssl")) {
        if (env->debug_)
            cout << "ServerOptions: ssl server \n";
        env->enable_ssl(); // search server.crt first, then <host>.<port>.crt
    }
#endif
}

ServerOptions::ServerOptions(int argc, char* argv[], ServerEnvironment* env)
    : ServerOptions(CommandLine(argc, argv), env) {
}

bool ServerOptions::help_option() const {
    if (vm_.count("help"))
        return true;
    return false;
}

bool ServerOptions::version_option() const {
    if (vm_.count("version"))
        return true;
    return false;
}
