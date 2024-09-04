/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/client/ClientEnvironment.hpp"

#include <iostream>
#include <iterator>
#include <stdexcept>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PasswdFile.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/TimeStamp.hpp"
#include "ecflow/core/Version.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif

using namespace ecf;
using namespace std;
using namespace boost;

// Provide upper and lower bounds for timeouts
/// The timeout is used control for how long we continue to iterate over the hosts
/// attempting to connect to the servers.
#ifdef DEBUG
    #define MAX_TIMEOUT 120            // Max time in seconds for client to deliver message
    #define DEFAULT_ZOMBIE_TIMEOUT 120 // Max time in seconds for client to deliver message
    #define MIN_TIMEOUT 5              // Some reasonable value
#else
    #define MAX_TIMEOUT 24 * 3600            // We don't try forever, only 24 hours
    #define DEFAULT_ZOMBIE_TIMEOUT 12 * 3600 // We don't try forever, only 12 hours
    #define MIN_TIMEOUT 10 * 60              // Some reasonable value
#endif

// #define DEBUG_ENVIRONMENT 1

template <class T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, "\n"));
    return os;
}

ClientEnvironment::ClientEnvironment(bool gui)
    : AbstractClientEnv(),
      timeout_(MAX_TIMEOUT),
      zombie_timeout_(DEFAULT_ZOMBIE_TIMEOUT),
      gui_(gui) {
    init();
}

ClientEnvironment::ClientEnvironment(bool gui, const std::string& host, const std::string& port)
    : AbstractClientEnv(),
      timeout_(MAX_TIMEOUT),
      zombie_timeout_(DEFAULT_ZOMBIE_TIMEOUT),
      gui_(gui) {
    init();
    set_host_port(host, port); // assumes we are NOT going to read host file
}

// test constructor
ClientEnvironment::ClientEnvironment(const std::string& hostFile, const std::string& host, const std::string& port)
    : AbstractClientEnv(),
      timeout_(MAX_TIMEOUT),
      zombie_timeout_(DEFAULT_ZOMBIE_TIMEOUT) {
    init();

    /// The host file passed in takes *precedence* over ECF_HOSTFILE environment variable read in init()
    host_file_ = hostFile;

    /// Override config, and environment.
    if (!host.empty()) {
        host_vec_.clear();
        host_vec_.emplace_back(host, port);
    }
}

void ClientEnvironment::init() {
    read_environment_variables();
#ifdef DEBUG_ENVIRONMENT
    std::cout << toString() << std::endl;
#endif

    // If no host specified default to local host and default port number
    if (host_vec_.empty())
        host_vec_.emplace_back(Str::LOCALHOST(), Str::DEFAULT_PORT_NUMBER());

    // Program option are read in last, and will override any previous setting
    // However program option are delayed until later. See notes in header
    if (debug_)
        std::cout << toString() << "\n";
}

void ClientEnvironment::set_debug(bool flag) {
    // show client environment when debug is enabled
    debug_ = flag;
    if (debug_)
        std::cout << toString() << "\n";
}

bool ClientEnvironment::get_next_host(std::string& errorMsg) {
    if (debug_)
        cout << "ClientEnvironment::get_next_host() host_file_read_ = " << host_file_read_
             << " host_file_ = " << host_file_ << "\n";
    if (!host_file_read_ && !host_file_.empty()) {

        if (!parseHostsFile(errorMsg)) {
            return false;
        }
        host_file_read_ = true;
        // SKIP OVER THE FIRST HOST, hence both paths need to increment host_vec_index_
    }

    host_vec_index_++;
    if (host_vec_index_ >= static_cast<int>(host_vec_.size())) {
        host_vec_index_ = 0;
    }

    return true;
}

// AbstractClientEnv functions: ============================================================================
const std::string& ClientEnvironment::host() const {
#ifdef DEBUG_ENVIRONMENT
// cout << "ClientEnvironment::host()  host_vec_index_ = " << host_vec_index_
//      << " host_vec_[host_vec_index_] = " << host_vec_[host_vec_index_] << "\n";
#endif
    assert(!host_vec_.empty());
    assert(host_vec_index_ >= 0 && host_vec_index_ < static_cast<int>(host_vec_.size()));
    return host_vec_[host_vec_index_].first;
}

const std::string& ClientEnvironment::port() const {
    assert(!host_vec_.empty());
    assert(host_vec_index_ >= 0 && host_vec_index_ < static_cast<int>(host_vec_.size()));
    return host_vec_[host_vec_index_].second;
}

void ClientEnvironment::set_host_port(const std::string& the_host, const std::string& the_port) {
    if (the_host.empty())
        throw std::runtime_error("ClientEnvironment::set_host_port: Empty host specified ?");
    if (the_port.empty())
        throw std::runtime_error("ClientEnvironment::set_host_port: Empty port specified ?");
    try {
        ecf::convert_to<int>(the_port);
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error("ClientEnvironment::set_host_port: Invalid port number " + the_port);
    }

    // Override default
    host_vec_.clear();

    // make sure there only one host:port in host_vec_
    host_vec_.emplace_back(the_host, the_port);

    // Make sure we don't look in hosts file.
    // When there is only one host:port in host_vec_, calling get_next_host() will always return host_vec_[0]
    host_file_read_ = true;

    // Caution:
    //
    //   We don't (re)enable SSL immediattelly after setting host/port, as this might happen multiple times
    //   during the execution (e.g. when loading environment variables, and later processing command line options).
    //
    //   It is up to the user of this class to enable SSL if needed.
    //
}

bool ClientEnvironment::checkTaskPathAndPassword(std::string& errorMsg) const {
    if (task_path_.empty()) {
        errorMsg = "No task path specified for ECF_NAME \n";
        return false;
    }
    if (jobs_password_.empty()) {
        errorMsg = "No jobs password specified for ECF_PASS \n";
        return false;
    }
    return true;
}

std::string ClientEnvironment::toString() const {
    std::stringstream ss;
    ss << "ClientEnvironment:\n";
    ss << TimeStamp::now() << Version::description() << "\n";
    ss << "   ECF_HOST/ECF_PORT : host_vec_index_ = " << host_vec_index_ << " host_vec_.size() = " << host_vec_.size()
       << "\n";
    if (!host_vec_.empty()) {
        for (std::pair<std::string, std::string> i : host_vec_) {
            ss << "   " << i.first << Str::COLON() << i.second << "\n";
        }
    }
    ss << "   ECF_NAME = " << task_path_ << "\n";
    ss << "   ECF_PASS = " << jobs_password_ << "\n";
    ss << "   ECF_RID = " << remote_id_ << "\n";
    ss << "   ECF_TRYNO = " << task_try_num_ << "\n";
    ss << "   ECF_HOSTFILE = " << host_file_ << "\n";
    ss << "   ECF_TIMEOUT = " << timeout_ << "\n";
    ss << "   ECF_ZOMBIE_TIMEOUT = " << zombie_timeout_ << "\n";
    ss << "   ECF_CONNECT_TIMEOUT = " << connect_timeout_ << "\n";
    ss << "   ECF_DENIED = " << denied_ << "\n";
    ss << "   NO_ECF = " << no_ecf_ << "\n";
    for (const auto& i : env_) {
        ss << "   " << i.first << " = " << i.second << "\n";
    }

    ss << "   ECF_DEBUG_CLIENT = " << debug_ << "\n";
#ifdef ECF_OPENSSL
    if (ssl())
        ss << "   ECF_SSL = " << ssl_ << "\n";
#endif
    return ss.str();
}

std::string ClientEnvironment::hostSpecified() {
    std::string specified_host;
    ecf::environment::get(ecf::environment::ECF_HOST, specified_host);
    return specified_host;
}

std::string ClientEnvironment::portSpecified() {
    std::string specified_port = Str::DEFAULT_PORT_NUMBER();
    ecf::environment::get(ecf::environment::ECF_PORT, specified_port);
    return specified_port;
}

void ClientEnvironment::read_environment_variables() {
#ifdef DEBUG_ENVIRONMENT
    std::cout << "ClientEnvironment::read_environment_variables()\n";
#endif

    ecf::environment::get(ecf::environment::ECF_NAME, task_path_);

    ecf::environment::get(ecf::environment::ECF_PASS, jobs_password_);

    ecf::environment::get(ecf::environment::ECF_TRYNO, task_try_num_);

    std::string host_protocol;
    ecf::environment::get(ecf::environment::ECF_HOST_PROTOCOL, host_protocol);
    if (host_protocol == "HTTP" || host_protocol == "HTTPS") {
        http_ = true;
    }

    ecf::environment::get("ECF_HOSTFILE", host_file_);

    ecf::environment::get(ecf::environment::ECF_RID, remote_id_);

    ecf::environment::get("ECF_USER", user_name_);

    ecf::environment::get("ECF_TIMEOUT", timeout_);
    timeout_ = timeout_ > MAX_TIMEOUT ? MAX_TIMEOUT : timeout_;
    timeout_ = timeout_ < MIN_TIMEOUT ? MIN_TIMEOUT : timeout_;

    ecf::environment::get("ECF_ZOMBIE_TIMEOUT", zombie_timeout_);
    zombie_timeout_ = (zombie_timeout_ > MAX_TIMEOUT) ? MAX_TIMEOUT : zombie_timeout_;
    zombie_timeout_ = (zombie_timeout_ < MIN_TIMEOUT) ? MIN_TIMEOUT : zombie_timeout_;

    ecf::environment::get("ECF_CONNECT_TIMEOUT", connect_timeout_);

    ecf::environment::get("ECF_DENIED", denied_);

    ecf::environment::get("NO_ECF", no_ecf_);

    ecf::environment::get("ECF_DEBUG_CLIENT", debug_);

    if (auto var = ecf::environment::fetch("ECF_DEBUG_LEVEL"); var) {
        try {
            Ecf::set_debug_level(ecf::convert_to<unsigned int>(var.value()));
        }
        catch (...) {
            throw std::runtime_error("The environment variable ECF_DEBUG_LEVEL must be an unsigned integer.");
        }
    }

    /// Override the config settings *IF* any
    std::string port = Str::DEFAULT_PORT_NUMBER();
    std::string host = Str::LOCALHOST();
    if (!host_vec_.empty()) {
        host = host_vec_[0].first;  //  first entry is the config host
        port = host_vec_[0].second; //  first entry is the config port
    }

    if (auto var = ecf::environment::fetch(ecf::environment::ECF_PORT); var) {
        port = var.value();
        host_vec_.clear(); // remove config settings, net effect is overriding
        host_vec_.emplace_back(host, port);
    }

    // Add the ECF_HOST host into list of hosts. Make sure its first in host_vec_
    // as we want environment setting to take priority.
    string env_host = hostSpecified();
    if (!env_host.empty()) {
        host = env_host;
        host_vec_.clear(); // remove previous setting if any
        host_vec_.emplace_back(host, port);
    }
}

bool ClientEnvironment::parseHostsFile(std::string& errorMsg) {
    std::vector<std::string> lines;
    if (!File::splitFileIntoLines(host_file_, lines, true /*IGNORE EMPTY LINES*/)) {
        std::stringstream ss;
        ss << "ClientEnvironment:: Could not open the hosts file " << host_file_;
        errorMsg += ss.str();
        return false;
    }

    // The Home server and port should be at index 0.
    std::string job_supplied_port = Str::DEFAULT_PORT_NUMBER();
    if (!host_vec_.empty())
        job_supplied_port = host_vec_[0].second;

    // Each line should have the format: We only read in the first token.
    //       host        # comment
    //       host:port   # another comment
    // If no port is specified we default to port read in from the environment
    // if that was not specified we default to Str::DEFAULT_PORT_NUMBER()
    auto theEnd = lines.end();
    for (auto i = lines.begin(); i != theEnd; ++i) {

        std::vector<std::string> tokens;
        Str::split((*i), tokens);
        if (tokens.empty())
            continue;
        if (tokens[0][0] == '#') {
            //			std::cout << "Ignoring line:'" << *i << "'\n";
            continue;
        }

        std::string theBackupHost;
        std::string thePort;

        size_t colonPos = tokens[0].find_first_of(':');
        if (colonPos == string::npos) {
            // When the port is *NOT* specified, we must use the job supplied port.
            // i.e. the one read in from the environment
            theBackupHost = tokens[0];
            thePort       = job_supplied_port;
            //			std::cout << "Host = " << theBackupHost << " Port is  " << thePort << "\n";
        }
        else {
            theBackupHost = tokens[0].substr(0, colonPos);
            thePort       = tokens[0].substr(colonPos + 1);
            //			std::cout << "Host = " << theBackupHost << " Port is specified " << thePort << "\n";
        }

        // std::cout << "Host = " << theBackupHost << "  " << thePort << "\n";
        host_vec_.emplace_back(theBackupHost, thePort);
    }

    return true;
}

const std::string& ClientEnvironment::get_user_password(const std::string& user) const {
    return get_password("ECF_PASSWD", user);
}

const std::string& ClientEnvironment::get_custom_user_password(const std::string& user) const {
    return get_password("ECF_CUSTOM_PASSWD", user);
}

const std::string& ClientEnvironment::get_password(const char* env, const std::string& user) const {
    // cout << "ClientEnvironment::get_password() env:" << env << " user:" << user << " passwd_(" << passwd_ << ")\n";
    if (user.empty())
        throw std::runtime_error("ClientEnvironment::get_user_password: No user specified");
    if (!passwd_.empty()) {
        // cout << "  ClientEnvironment::get_password() CACHED returning " << passwd_ << "\n";
        return passwd_;
    }

    if (auto file = ecf::environment::fetch(env); file) {
        std::string user_passwd_file = file.value();
        // cout << "  ClientEnvironment::get_password() ECF_CUSTOM_PASSWD " << user_passwd_file  << "\n";
        if (!user_passwd_file.empty() && fs::exists(user_passwd_file)) {
            // cout << "  ClientEnvironment::get_password() LOADING password file\n";
            PasswdFile passwd_file;
            std::string errorMsg;
            if (!passwd_file.load(user_passwd_file, debug(), errorMsg)) {
                std::stringstream ss;
                ss << "Could not parse ECF_CUSTOM_PASSWD file. " << errorMsg;
                throw std::runtime_error(ss.str());
            }
            // std::cout << "ClientEnvironment::get_password() PasswdFile.dump()\n" << passwd_file.dump() << "\n";
            // std::cout << "ClientEnvironment::get_password() user(" << user << ")" << " host(" << host() << ") port("
            // << port() << ")\n";

            passwd_ = passwd_file.get_passwd(user, host(), port()); // host() may return localhost.
            if (passwd_.empty()) {
                Host host;
                passwd_ = passwd_file.get_passwd(user, host.name(), port());
            }
            // cout << "  ClientEnvironment::get_password() returning(" << passwd_ << ")\n";
            return passwd_;
        }
    }

    // cout << "  ClientEnvironment::get_user_password() returning EMPTY \n";
    return Str::EMPTY();
}
