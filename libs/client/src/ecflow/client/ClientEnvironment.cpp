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

#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "ecflow/client/HostsFile.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Enumerate.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PasswdFile.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/TimeStamp.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif

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

static constexpr const char* ECF_HOSTFILE_POLICY_ALL  = "all";
static constexpr const char* ECF_HOSTFILE_POLICY_TASK = "task";

template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, "\n"));
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
    if (host_vec_.empty()) {
        host_vec_.emplace_back(ecf::Str::LOCALHOST(), ecf::Str::DEFAULT_PORT_NUMBER());
    }

    // Program options are read in last, and will override any previous setting
    // However, the program options are delayed until later. See notes in header
    if (debug_) {
        std::cout << toString() << "\n";
    }
}

void ClientEnvironment::set_debug(bool flag) {
    debug_ = flag;

    // Showing the client environment is a side effect of enabling debug
    if (debug_) {
        std::cout << toString() << "\n";
    }
}

bool ClientEnvironment::get_next_host(std::string& errorMsg) {
    if (debug_) {
        std::cout << "ClientEnvironment::get_next_host() host_file_read_ = " << host_file_read_
                  << " host_file_ = " << host_file_ << "\n";
    }
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

const std::string& ClientEnvironment::host() const {
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
    if (the_host.empty()) {
        THROW_RUNTIME("ClientEnvironment::set_host_port: Empty host specified ?");
    }
    if (the_port.empty()) {
        THROW_RUNTIME("ClientEnvironment::set_host_port: Empty port specified ?");
    }

    try {
        ecf::convert_to<int>(the_port);
    }
    catch (const ecf::bad_conversion&) {
        THROW_RUNTIME("ClientEnvironment::set_host_port: Invalid port number " << the_port);
    }

    // Override default
    host_vec_.clear();

    // make sure there only one host:port in host_vec_
    host_vec_.emplace_back(the_host, the_port);

    //
    // Even when --host/--port options are used, we still allow ECF_HOSTFILE to be processed
    //
    //   This allows to:
    //     1. use host/port as defined per ECF_HOST/ECP_PORT
    //     2. override the host/port with the option --host/--port
    //     and, still use the content of ECF_HOSTFILE as alternative hosts to try
    //
    host_file_read_ = false;

    // Caution:
    //
    //   We don't (re)enable SSL immediately after setting host/port, as this might happen multiple times
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
    ss << ecf::TimeStamp::now() << ecf::Version::description() << "\n";
    ss << "   ECF_HOST/ECF_PORT : host_vec_index_ = " << host_vec_index_ << " host_vec_.size() = " << host_vec_.size()
       << "\n";
    if (!host_vec_.empty()) {
        for (std::pair<std::string, std::string> i : host_vec_) {
            ss << "   " << i.first << ecf::Str::COLON() << i.second << "\n";
        }
    }
    ss << "   " << ecf::environment::ECF_NAME << " = " << task_path_ << "\n";
    ss << "   " << ecf::environment::ECF_PASS << " = " << jobs_password_ << "\n";
    ss << "   " << ecf::environment::ECF_RID << " = " << remote_id_ << "\n";
    ss << "   " << ecf::environment::ECF_TRYNO << " = " << task_try_num_ << "\n";
    ss << "   " << ecf::environment::ECF_HOSTFILE << " = " << host_file_ << "\n";
    ss << "   " << ecf::environment::ECF_HOSTFILE_POLICY << " = " << host_file_policy_ << "\n";
    ss << "   " << ecf::environment::ECF_TIMEOUT << " = " << timeout_ << "\n";
    ss << "   " << ecf::environment::ECF_ZOMBIE_TIMEOUT << " = " << zombie_timeout_ << "\n";
    ss << "   " << ecf::environment::ECF_CONNECT_TIMEOUT << " = " << connect_timeout_ << "\n";
    ss << "   " << ecf::environment::ECF_DENIED << " = " << denied_ << "\n";
    ss << "   " << ecf::environment::NO_ECF << " = " << no_ecf_ << "\n";
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
    std::string specified_port = ecf::Str::DEFAULT_PORT_NUMBER();
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

    if (auto protocol = ecf::environment::fetch(ecf::environment::ECF_HOST_PROTOCOL); protocol) {
        auto found = ecf::Enumerate<ecf::Protocol>::to_enum(protocol.value());
        protocol_  = found ? found.value() : ecf::Protocol::Plain;
    }

    ecf::environment::get(ecf::environment::ECF_HOSTFILE, host_file_);

    host_file_policy_ = ECF_HOSTFILE_POLICY_TASK;
    ecf::environment::get(ecf::environment::ECF_HOSTFILE_POLICY, host_file_policy_);
    // ensure the policy is lower case
    ecf::algorithm::tolower(host_file_policy_);
    // ensure the policy is either "all" or "task", otherwise default to "task"
    if (host_file_policy_ != ECF_HOSTFILE_POLICY_ALL && host_file_policy_ != ECF_HOSTFILE_POLICY_TASK) {
        host_file_policy_ = ECF_HOSTFILE_POLICY_TASK;
    }

    ecf::environment::get(ecf::environment::ECF_RID, remote_id_);

    ecf::environment::get(ecf::environment::ECF_USER, user_name_);

    ecf::environment::get(ecf::environment::ECF_TIMEOUT, timeout_);
    timeout_ = timeout_ > MAX_TIMEOUT ? MAX_TIMEOUT : timeout_;
    timeout_ = timeout_ < MIN_TIMEOUT ? MIN_TIMEOUT : timeout_;

    ecf::environment::get(ecf::environment::ECF_ZOMBIE_TIMEOUT, zombie_timeout_);
    zombie_timeout_ = (zombie_timeout_ > MAX_TIMEOUT) ? MAX_TIMEOUT : zombie_timeout_;
    zombie_timeout_ = (zombie_timeout_ < MIN_TIMEOUT) ? MIN_TIMEOUT : zombie_timeout_;

    ecf::environment::get(ecf::environment::ECF_CONNECT_TIMEOUT, connect_timeout_);

    ecf::environment::get(ecf::environment::ECF_DENIED, denied_);

    ecf::environment::get(ecf::environment::NO_ECF, no_ecf_);

    ecf::environment::get(ecf::environment::ECF_DEBUG_CLIENT, debug_);

    if (auto var = ecf::environment::fetch(ecf::environment::ECF_DEBUG_LEVEL); var) {
        try {
            Ecf::set_debug_level(ecf::convert_to<unsigned int>(var.value()));
        }
        catch (...) {
            THROW_RUNTIME("The environment variable ECF_DEBUG_LEVEL must be an unsigned integer.");
        }
    }

    /// Override the config settings *IF* any
    std::string port = ecf::Str::DEFAULT_PORT_NUMBER();
    std::string host = ecf::Str::LOCALHOST();
    if (!host_vec_.empty()) {
        const auto& selected = host_vec_[0]; //  the first entry holds the selected host/port
        host                 = selected.first;
        port                 = selected.second;
    }

    if (auto var = ecf::environment::fetch(ecf::environment::ECF_PORT); var) {
        port = var.value();
        host_vec_.clear(); // remove config settings, net effect is overriding
        host_vec_.emplace_back(host, port);
    }

    // Add the ECF_HOST host into the list of hosts. Make sure its first in host_vec_
    // as we want environment setting to take priority.
    std::string env_host = hostSpecified();
    if (!env_host.empty()) {
        host = env_host;
        host_vec_.clear(); // remove previous setting if any
        host_vec_.emplace_back(host, port);
    }
}

bool ClientEnvironment::parseHostsFile(std::string& errorMsg) {

    std::string configured_port = host_vec_.empty() ? ecf::Str::DEFAULT_PORT_NUMBER() : host_vec_[0].second;
    int port                    = ecf::convert_to<int>(configured_port);

    try {
        auto alternate_hosts_file   = ecf::HostsFile::parse(host_file_, port);
        const auto& alternate_hosts = alternate_hosts_file.hosts();

        using std::begin, std::end;
        host_vec_.insert(end(host_vec_), begin(alternate_hosts), end(alternate_hosts));
    }
    catch (const ecf::HostsFileFailure& e) {
        errorMsg = e.what();
        return false;
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
    if (user.empty()) {
        THROW_RUNTIME("ClientEnvironment::get_user_password: No user specified");
    }
    if (!passwd_.empty()) {
        return passwd_;
    }

    if (auto file = ecf::environment::fetch(env); file) {
        std::string user_passwd_file = file.value();
        if (!user_passwd_file.empty() && fs::exists(user_passwd_file)) {
            PasswdFile passwd_file;
            std::string errorMsg;
            if (!passwd_file.load(user_passwd_file, debug(), errorMsg)) {
                THROW_RUNTIME("Could not parse ECF_CUSTOM_PASSWD file. " << errorMsg);
            }

            passwd_ = passwd_file.get_passwd(user, host(), port()); // host() may return localhost.
            if (passwd_.empty()) {
                ecf::Host host;
                passwd_ = passwd_file.get_passwd(user, host.name(), port());
            }
            return passwd_;
        }
    }

    return ecf::Str::EMPTY();
}

struct TokenFile
{
public:
    static TokenFile load(const fs::path& path) {
        using nlohmann::json;
        using namespace std::string_literals;

        std::ifstream f(path.c_str());

        if (!f.good()) {
            throw UnavailableToken("Unable to open : "s + path.c_str() + ", to load tokens.");
        }

        try {
            json data = json::parse(f);

            // Check version
            if (auto meta_version = data["version"]; meta_version.is_number_integer()) {
                if (auto version = int(meta_version); version != 1) {
                    throw UnavailableToken("Unable to load tokens, from: "s + path.c_str() +
                                           ". Expected version 1, found:" + std::to_string(version));
                }
            }
            else {
                throw UnavailableToken("Unable to load tokens, from: "s + path.c_str() + ". No version found.");
            }

            TokenFile file;
            // Load tokens
            for (auto token : data["tokens"]) {
                auto type = token["type"];
                if (type == "bearer") {
                    auto server = std::string(token["server"]);
                    auto url    = std::string(token["api"]["url"]);
                    auto key    = std::string(token["api"]["key"]);
                    auto email  = std::string(token["api"]["email"]);
                    file.tokens.push_back(Token::make_bearer(server, url, key, email));
                }
                else if (type == "basic") {
                    auto server   = std::string(token["server"]);
                    auto username = std::string(token["api"]["username"]);
                    auto password = std::string(token["api"]["password"]);
                    file.tokens.push_back(Token::make_basic(server, username, password));
                }
            }
            return file;
        }
        catch (const nlohmann::detail::exception& e) {
            using namespace std::string_literals;
            throw UnavailableToken("Unable to load tokens, from: "s + path.c_str() + ". Due to: " + e.what());
        }
        catch (const std::exception& e) {
            using namespace std::string_literals;
            throw UnavailableToken("Unable to load tokens, from: "s + path.c_str() + ". Due to: " + e.what());
        }
    }

    std::vector<Token> tokens;
};

fs::path get_ecflow_api_tokens_path() {
    const char* const ECFLOWAPIRC = ".ecflowapirc";

    // Use the value of ${ECF_AUTHTOKENS} environment variable, if specified
    const char* const ECF_AUTHTOKENS = "ECF_AUTHTOKENS";
    if (auto selected_path = ecf::environment::fetch(ECF_AUTHTOKENS); selected_path) {
        std::string tokens_path = selected_path.value();
        return fs::absolute(tokens_path);
    }

    // Otherwise, search for ${HOME}/.ecflowapirc
    const char* const HOME = "HOME";
    if (auto base_path = ecf::environment::fetch(HOME); base_path) {
        std::string tokens_path = base_path.value() + "/" + ECFLOWAPIRC;
        if (fs::exists(tokens_path)) {
            return fs::absolute(tokens_path);
        }
    }

    // As an ultimate fallback, use the current directory (i.e. ${PWD}/.ecflowapirc)
    std::string tokens_path = ECFLOWAPIRC;
    return fs::absolute(tokens_path);
}

Token ClientEnvironment::get_token(const std::string& url) const {
    using namespace std::string_literals;

    auto tokens_path = get_ecflow_api_tokens_path();

    try {
        auto data = TokenFile::load(tokens_path);
        for (auto token : data.tokens) {
            std::regex pattern(token.server);
            if (std::regex_match(url, pattern)) {
                return token;
            }
        }
    }
    catch (const UnavailableToken& e) {
        throw;
    }

    throw UnavailableToken("Unable to find token for URL: "s + url);
}

bool ClientEnvironment::host_file_policy_is_task() const {
    // If the host file policy is not set or is set to "task", then we use the task policy.
    return host_file_policy_.empty() or host_file_policy_ == ECF_HOSTFILE_POLICY_TASK;
}

bool ClientEnvironment::host_file_policy_is_all() const {
    // If the host file policy is set to "all", then we use the all policy.
    return host_file_policy_ == ECF_HOSTFILE_POLICY_ALL;
}
