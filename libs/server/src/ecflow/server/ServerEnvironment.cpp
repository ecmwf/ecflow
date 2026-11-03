// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/server/ServerEnvironment.hpp"

#include <iostream>
#include <map>
#include <optional>
#include <vector>

#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/MaskCredentials.hpp"
#include "ecflow/core/Pid.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/core/cfg/CfgReader.hpp"
#include "ecflow/core/cfg/CliReader.hpp"
#include "ecflow/core/cfg/EnvReader.hpp"
#include "ecflow/core/cfg/JsonReader.hpp"
#include "ecflow/core/cfg/Mapper.hpp"
#include "ecflow/core/cfg/ServerCatalogue.hpp"
#include "ecflow/node/JobProfiler.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/server/ServerOptions.hpp"

using namespace ecf;

static std::string the_check_mode(ecf::CheckPt::Mode mode) {
    switch (mode) {
        case ecf::CheckPt::NEVER:
            return "CHECK_NEVER";
        case ecf::CheckPt::ON_TIME:
            return "CHECK_ON_TIME";
        case ecf::CheckPt::ALWAYS:
            return "CHECK_ALWAYS";
        case ecf::CheckPt::UNDEFINED:
            return "UNDEFINED";
    }
    std::cout << "ServerEnvironment.cpp theCheckMode: assert failed\n";
    assert(false);
    return std::string{};
}

// Describes how clients connect, e.g. "TCP/IP v4", "TCP/IP v4 with SSL", "HTTP over TCP/IP v6"
static std::string the_connection(ecf::Protocol protocol, const boost::asio::ip::tcp& tcp) {
    const std::string transport = (tcp.family() == 2 /*PF_INET*/) ? "TCP/IP v4" : "TCP/IP v6";
    switch (protocol) {
        case ecf::Protocol::Plain:
            return transport;
        case ecf::Protocol::Ssl:
            return transport + " with SSL";
        case ecf::Protocol::Http:
            return "HTTP over " + transport;
        case ecf::Protocol::Https:
            return "HTTP over " + transport + " with SSL";
    }
    return transport;
}

////////////////////////////////////////////////////////////////////////////////////////////
// class ServerEnvironment:
///////////////////////////////////////////////////////////////////////////////////////////

ServerEnvironment::ServerEnvironment(const CommandLine& cl, const std::string& path_to_config_file)
    : serverHost_(host_name_.name()),
      serverPort_(0),
      checkPtInterval_(0),
      checkpt_save_time_alarm_(CheckPt::default_save_time_alarm()),
      submitJobsInterval_(Ecf::SUBMIT_INTERVAL()),
      ecf_prune_node_log_(0),
      ecf_task_threshold_(Ecf::TASK_THRESHOLD()),
      jobGeneration_(true),
      protocol_{ecf::Protocol::Plain},
      debug_(false),
      help_option_(false),
      version_option_(false),
      checkMode_(ecf::CheckPt::ON_TIME),
      tcp_protocol_(boost::asio::ip::tcp::v4()) {
    Ecf::set_server(true);

    init(cl, path_to_config_file);
}

void ServerEnvironment::init(const CommandLine& cl, const std::string& path_to_config_file) {
    ServerOptions options(cl);
    help_option_    = options.help_option();
    version_option_ = options.version_option();
    if (help_option_ || version_option_) {
        return;
    }

    std::string log_file_name;
    std::optional<ecf::cfg::Configuration> configuration;
    try {
        configuration.emplace(read_configuration(options, path_to_config_file));
        resolve(*configuration, log_file_name);
    }
    catch (std::exception& e) {
        std::string msg           = "Exception in ServerEnvironment::ServerEnvironment() : ";
        std::string exception_msg = e.what();
        // On ecgate, wrong locale, causes an exception where msg is empty
        if (exception_msg.empty()) {
            msg += "Invalid locale? Check locale using 'locale -a', then export/set LANG environment";
        }
        else {
            msg += e.what();
        }
        throw ServerEnvironmentException(msg);
    }

    // get server process id. This may be visualised in xecf. makes it easier to kill server
    try {
        ecf_pid_ = Pid::getpid();
    }
    catch (...) {
        throw ServerEnvironmentException("ServerEnvironment::ServerEnvironment:: Could not convert PID to a string\n");
    }
    // std::cout << "PID = " << ecf_pid_ << "\n";

#ifdef ECF_OPENSSL
    select_ssl(*configuration);
#endif

    JobProfiler::set_task_threshold(static_cast<size_t>(ecf_task_threshold_));

    /// Config, Environment, or Options may have updated port, update port dependent file names
    /// If we have default names make unique, by prefixing host and port
    std::string port = ecf::convert_to<std::string>(serverPort_);

    // If path is absolute leave as is
    if (ecf_checkpt_file_ == Ecf::CHECKPT()) {
        ecf_checkpt_file_ = host_name_.prefix_host_and_port(port, ecf_checkpt_file_);
    }
    if (ecf_checkpt_file_[0] != '/') {
        // Prepend with ECF_HOME
        std::string check_pt = ecf_home();
        check_pt += ecf::string_constants::path_separator;
        check_pt += ecf_checkpt_file_;
        ecf_checkpt_file_ = check_pt;
    }

    // If path is absolute leave as is
    if (ecf_backup_checkpt_file_ == Ecf::BACKUP_CHECKPT()) {
        ecf_backup_checkpt_file_ = host_name_.prefix_host_and_port(port, ecf_backup_checkpt_file_);
    }
    if (ecf_backup_checkpt_file_[0] != '/') {
        std::string check_pt = ecf_home();
        check_pt += ecf::string_constants::path_separator;
        check_pt += ecf_backup_checkpt_file_;
        ecf_backup_checkpt_file_ = check_pt;
    }

    if (ecf_white_list_file_ == ecf::string_constants::white_list_file) {
        ecf_white_list_file_ = host_name_.prefix_host_and_port(port, ecf_white_list_file_);
    }

    authentication_service_.init(host_name_, port);

    // Change directory to ECF_HOME and check that it is accessible
    change_dir_to_ecf_home_and_check_accesibility();

    // LOG FILE ================================================================================
    if (log_file_name == Ecf::LOG_FILE()) {
        log_file_name = host_name_.prefix_host_and_port(port, log_file_name);
    }

    // Create the Log file.
    //
    // The log file is obtained from the environment, hence, **must** be done last.
    // From ecflow version 4.9.0 we no longer flush for each command.
    Log::create(log_file_name);

    // Init log file:
    LOG(Log::MSG, ""); // previous log may not end in newline

    // The start time, version and runtime configuration, written to the log, and to the standard output when
    // debugging
    report(
        {"Started at " + to_simple_string(Calendar::second_clock_time()) + " universal time", Version::description()});
    report(runtime_configuration_lines());

#ifdef ECF_OPENSSL
    // Warn when the certificate in use is not the one requested first
    if (ssl_.enabled()) {
        if (const auto& missing = ssl_.fallback_from()) {
            LOG(Log::WAR,
                "SSL certificate '" << missing->string() << "' not found; using the "
                                    << (ssl_.certificate()->shared() ? "shared" : "server-specific")
                                    << " certificate instead");
        }
    }
#endif
}

ServerEnvironment::~ServerEnvironment() {
    /// Destroy singleton to avoid valgrind from complaining
    Log::destroy();
    System::destroy();
}

bool ServerEnvironment::valid(std::string& errorMsg) const {
    /// This must be called *AFTER* the constructor
    //   cout << "*****************************************************************************\n";
    //   cout << "ServerEnvironment::valid\n";
    //   cout << "*****************************************************************************\n";

    if (serverHost_.empty()) {
        errorMsg = "Could not determine the server host.";
        return false;
    }
    std::ostringstream ss;
    if (!Log::instance() || Log::instance()->path().empty()) {
        ss << "No log file name specified. Please set in Server/server_environment.cfg or\n";
        ss << "set the environment variable ECF_LOG\n";
        errorMsg = ss.str();
        return false;
    }
    // The permissions of the nodes apply, unless replaced by those of a whitelist file
    if (auto result = AuthorisationService::load_permissions_from_nodes(); result.ok()) {
        authorisation_service_ = result.value();
    }

    // Check that Authentication information is valid
    if (auto status = authentication_service_.valid(serverHost_, the_port(), errorMsg); !status) {
        return false;
    }
    access_loaded_ = true;
    log_authentication();

    // *WHEN* the server starts, it is possible that the white list file is empty or simply does not exist, since any
    // user is free to access the server.
    if (ecf_white_list_file_.empty() || !fs::exists(ecf_white_list_file_)) {
        log_authorisation();
        return true;
    }

    /// read in the ecf white list file that specifies valid users and their access rights
    /// If the file cannot be opened returns false and an error message and false;
    /// Automatically add server admin(user) with write access, as this will allow admin reload
    return load_whitelist_file(errorMsg);
}

std::pair<std::string, std::string> ServerEnvironment::hostPort() const {
    return std::make_pair(serverHost_, the_port());
}

std::string ServerEnvironment::the_port() const {
    return ecf::convert_to<std::string>(serverPort_);
}

void ServerEnvironment::variables(std::vector<std::pair<std::string, std::string>>& theRetVec) const {
    // Variables read in from the environment
    // Need to setup client environment.
    // The server sets these variable for use by the client, i.e. when creating the jobs
    // The clients then uses them to communicate back with the server.
    theRetVec.emplace_back(ecf::environment::ECF_PORT, the_port());
    theRetVec.emplace_back(ecf::environment::ECF_HOST, serverHost_);

    theRetVec.emplace_back(ecf::environment::ECF_HOME, ecfHome_);
    theRetVec.emplace_back(std::string("ECF_LOG"), Log::instance()->path());
    theRetVec.emplace_back(std::string("ECF_CHECK"), ecf_checkpt_file_);
    theRetVec.emplace_back(std::string("ECF_CHECKOLD"), ecf_backup_checkpt_file_);
    theRetVec.emplace_back(std::string("ECF_INTERVAL"), ecf::convert_to<std::string>(submitJobsInterval_));

    // These variable are read in from the environment, but are not exposed
    // since they only affect the server
    // ECF_CHECKINTERVAL

    // read only variables, changing them has no effect
    theRetVec.emplace_back(std::string("ECF_LISTS"), ecf_white_list_file_);
    theRetVec.emplace_back(std::string("ECF_PASSWD"), authentication_service_.passwd_file());
    theRetVec.emplace_back(std::string("ECF_CUSTOM_PASSWD"), authentication_service_.custom_passwd_file());

    // variables that can be overridden, in the suite definition
    theRetVec.emplace_back(std::string("ECF_JOB_CMD"), ecf_cmd_);
    theRetVec.emplace_back(std::string("ECF_KILL_CMD"), killCmd_);
    theRetVec.emplace_back(std::string("ECF_STATUS_CMD"), statusCmd_);
    theRetVec.emplace_back(std::string("ECF_CHECK_CMD"), checkCmd_);
    theRetVec.emplace_back(std::string("ECF_URL_CMD"), urlCmd_);
    theRetVec.emplace_back(std::string("ECF_URL_BASE"), urlBase_);
    theRetVec.emplace_back(std::string("ECF_URL"), url_);
    theRetVec.emplace_back(std::string("ECF_MICRO"), ecf_micro_);

    // Reference variable, these should be read only
    theRetVec.emplace_back(std::string("ECF_PID"), ecf_pid_);            // server PID
    theRetVec.emplace_back(std::string("ECF_VERSION"), Version::full()); // server version

#ifdef ECF_OPENSSL
    if (ssl_.enabled()) {
        theRetVec.emplace_back(std::string("ECF_SSL"), ssl_.certificate()->to_envvar_value());
    }
#endif
}

bool ServerEnvironment::reloadWhiteListFile(std::string& errorMsg) {
    if (debug()) {
        std::cout << "ServerEnvironment::reloadWhiteListFile:(" << ecf_white_list_file_ << ") CWD("
                  << fs::current_path().string() << ")\n";
    }
    if (ecf_white_list_file_.empty()) {
        errorMsg += "The ECF_LISTS file ";
        errorMsg += ecf_white_list_file_;
        errorMsg += " has not been specified.";
        return false;
    }
    if (!fs::exists(ecf_white_list_file_)) {
        errorMsg += "The ECF_LISTS file ";
        errorMsg += ecf_white_list_file_;
        errorMsg += " does not exist. Server CWD : " + fs::current_path().string();
        return false;
    }

    return load_whitelist_file(errorMsg);
}

bool ServerEnvironment::load_whitelist_file(std::string& errorMsg) const {
    if (!fs::is_regular_file(ecf_white_list_file_)) {
        errorMsg += "ECF_LISTS path '" + ecf_white_list_file_ + "' is not a regular file";
        return false;
    }

    WhiteListFile replacement;
    std::string parse_error;
    if (replacement.load(ecf_white_list_file_, parse_error)) {
        // If the user accidentally removes the server/user from whitelist,
        // they will not be able to reload the whitelist, since it requires write access.
        // (Requires terminate, modify white list, restart to fix)
        // Hence always allow server user write access *IF* required for non-empty file
        replacement.allow_write_access_for_server_user();

        // Authorisation retains a reference to the stable member, not to the temporary replacement; building it
        // from a whitelist cannot fail
        white_list_file_.swap(replacement);
        authorisation_service_ = AuthorisationService::load_permissions_from_whitelist(white_list_file_).value();
        log_authorisation();
        return true;
    }
    else {
        errorMsg += "Could not load ECF_LISTS file '" + ecf_white_list_file_ + "': " + parse_error;
    }
    return false;
}

void ServerEnvironment::log_authentication() const {
    report(authentication_lines());
}

void ServerEnvironment::log_authorisation() const {
    report(authorisation_lines());
}

std::vector<std::string> ServerEnvironment::authentication_lines() const {
    std::vector<std::string> lines{"Authentication:"};
    const auto files = authentication_service_.describe();
    lines.insert(lines.end(), files.begin(), files.end());
    return lines;
}

std::vector<std::string> ServerEnvironment::authorisation_lines() const {
    // The whitelist, once loaded, replaces the permissions of the nodes
    const bool whitelist = !white_list_file_.path().empty();

    std::vector<std::string> lines{"Authorisation:", std::string("  Mode = ") + (whitelist ? "whitelist" : "nodes")};
    if (ecf_white_list_file_.empty()) {
        lines.emplace_back("  ECF_LISTS = not set");
    }
    else {
        lines.push_back("  ECF_LISTS (" + fs::absolute(ecf_white_list_file_).lexically_normal().string() +
                        ") = " + (whitelist ? "loaded" : "absent"));
    }
    if (whitelist) {
        lines.emplace_back("  Users:");
        for (const auto& rule : white_list_file_.access_rules()) {
            lines.push_back("    " + rule);
        }
    }
    return lines;
}

void ServerEnvironment::report(const std::vector<std::string>& lines) const {
    for (const auto& line : lines) {
        LOG(Log::MSG, line);
        if (debug_) {
            std::cout << line << "\n";
        }
    }
}

std::vector<std::string> ServerEnvironment::runtime_configuration_lines() const {
    // The sources read, then the connection, locations, scheduling and check-pointing
    // settings, commands and switches of the server, one entry per line, each with the source of its value. Commands
    // and URLs are set by the operator, and may hold credentials, which are masked.
    std::vector<std::string> lines{"Runtime Configuration:", "  Sources:"};
    for (const auto& source : configuration_sources_) {
        lines.push_back("    " + source.first + " = " + source.second);
    }

    // A value derived by the server, rather than read from a source, is internal; it is given no name
    auto origin = [this](const std::string& name) {
        const auto found = configuration_origins_.find(name);
        return found == configuration_origins_.end() ? std::string("internal") : found->second;
    };
    auto entry = [&lines, &origin](const std::string& key, const std::string& value, const std::string& name) {
        lines.push_back("    " + key + " = " + value + " @ " + origin(name));
    };

    lines.emplace_back("  Values:");
    entry("Host", hostPort().first, {});
    entry("Port", hostPort().second, "ECF_PORT");
    entry("Connection", the_connection(protocol_, tcp_protocol_), {});

#ifdef ECF_OPENSSL
    // Record the certificate in use
    if (ssl_.enabled()) {
        entry("SSL certificate",
              ssl_.certificate()->crt_file().string() + " (" +
                  (ssl_.certificate()->shared() ? "shared" : "server-specific") + ")",
              configuration_origins_.count("ECF_SSL") ? "ECF_SSL" : "--ssl");
    }
#endif

    entry("ECF_HOME", ecfHome_, "ECF_HOME");
    entry("ECF_LOG", Log::instance()->path(), "ECF_LOG");
    entry("ECF_CHECK", ecf_checkpt_file_, "ECF_CHECK");
    entry("ECF_CHECKOLD", ecf_backup_checkpt_file_, "ECF_CHECKOLD");
    entry("ECF_LISTS", ecf_white_list_file_, "ECF_LISTS");
    entry("ECF_PASSWD", authentication_service_.passwd_file(), "ECF_PASSWD");
    entry("ECF_CUSTOM_PASSWD", authentication_service_.custom_passwd_file(), "ECF_CUSTOM_PASSWD");
    entry("ECF_CHECKINTERVAL", std::to_string(checkPtInterval_), "ECF_CHECKINTERVAL");
    entry("ECF_CHECKMODE", the_check_mode(checkMode_), "ECF_CHECKMODE");
    entry("ECF_PRUNE_NODE_LOG", std::to_string(ecf_prune_node_log_), "ECF_PRUNE_NODE_LOG");
    entry("ECF_TASK_THRESHOLD", std::to_string(JobProfiler::task_threshold()), "ECF_TASK_THRESHOLD");
    entry("ECF_DEBUG_SERVER", debug_ ? "enabled" : "disabled", "ECF_DEBUG_SERVER");
    entry("ECF_JOB_CMD", ecf::mask_credentials(ecf_cmd_), "ECF_JOB_CMD");
    entry("ECF_KILL_CMD", ecf::mask_credentials(killCmd_), "ECF_KILL_CMD");
    entry("ECF_STATUS_CMD", ecf::mask_credentials(statusCmd_), "ECF_STATUS_CMD");
    entry("ECF_CHECK_CMD", ecf::mask_credentials(checkCmd_), "ECF_CHECK_CMD");
    entry("ECF_URL_CMD", ecf::mask_credentials(urlCmd_), "ECF_URL_CMD");
    entry("ECF_URL_BASE", ecf::mask_credentials(urlBase_), "ECF_URL_BASE");
    entry("ECF_URL", ecf::mask_credentials(url_), "ECF_URL");
    entry("ECF_MICRO", ecf::mask_credentials(ecf_micro_), "ECF_MICRODEF");
    entry("Job scheduling interval", std::to_string(submitJobsInterval_), "ECF_INTERVAL");
    entry("Job generation", jobGeneration_ ? "enabled" : "disabled", "--dis_job_gen");
    entry("Checkpoint save time alarm", std::to_string(checkpt_save_time_alarm_), {});
    for (const auto& [name, value] : configuration_switches_) {
        entry(name, value ? "true" : "false", name);
    }
    return lines;
}

// ============================================================================================
// Privates:
// ============================================================================================

void ServerEnvironment::resolve(const ecf::cfg::Configuration& configuration, std::string& log_file_name) {
    // Every setting used here has a default, hence an effective value
    auto value = [&configuration](const char* name) {
        return *configuration.get(ecf::cfg::Key::for_variable("ecflow.server.variables", name));
    };
    auto option = [&configuration](const char* path) {
        return std::get<bool>(*configuration.get(ecf::cfg::Key::for_option(path)));
    };
    auto text    = [&value](const char* name) { return std::get<std::string>(value(name)); };
    auto integer = [&value](const char* name) { return static_cast<int>(std::get<std::int64_t>(value(name))); };

    ecfHome_                 = text("ECF_HOME");
    serverPort_              = integer("ECF_PORT");
    ecf_checkpt_file_        = text("ECF_CHECK");
    ecf_backup_checkpt_file_ = text("ECF_CHECKOLD");
    log_file_name            = text("ECF_LOG");
    checkPtInterval_         = integer("ECF_CHECKINTERVAL");
    submitJobsInterval_      = integer("ECF_INTERVAL");
    ecf_cmd_                 = text("ECF_JOB_CMD");
    killCmd_                 = text("ECF_KILL_CMD");
    statusCmd_               = text("ECF_STATUS_CMD");
    checkCmd_                = text("ECF_CHECK_CMD");
    urlCmd_                  = text("ECF_URL_CMD");
    urlBase_                 = text("ECF_URL_BASE");
    url_                     = text("ECF_URL");
    ecf_micro_               = text("ECF_MICRODEF");
    ecf_white_list_file_     = text("ECF_LISTS");
    ecf_task_threshold_      = integer("ECF_TASK_THRESHOLD");
    ecf_prune_node_log_      = integer("ECF_PRUNE_NODE_LOG");
    debug_                   = std::get<bool>(value("ECF_DEBUG_SERVER"));
    jobGeneration_           = option("ecflow.server.job_generation");
    if (option("ecflow.server.http")) {
        protocol_ = ecf::Protocol::Http;
    }
    if (option("ecflow.server.ipv6")) {
        tcp_protocol_ = boost::asio::ip::tcp::v6();
    }
    authentication_service_.set_passwd_file(text("ECF_PASSWD"));
    authentication_service_.set_custom_passwd_file(text("ECF_CUSTOM_PASSWD"));

    // The catalogue accepts only the supported modes
    if (const auto mode = text("ECF_CHECKMODE"); mode == "CHECK_NEVER") {
        checkMode_ = ecf::CheckPt::NEVER;
    }
    else if (mode == "CHECK_ALWAYS") {
        checkMode_ = ecf::CheckPt::ALWAYS;
    }
    else {
        checkMode_ = ecf::CheckPt::ON_TIME;
    }

    // Resolve the server home against the startup directory
    if (ecfHome_ == ".") {
        ecfHome_ = fs::current_path().string();
    }
    else if (fs::path(ecfHome_).is_relative()) {
        auto home = fs::absolute(ecfHome_).lexically_normal();
        if (home.filename().empty() && home.has_relative_path()) {
            home = home.parent_path(); // drop the trailing separator
        }
        ecfHome_ = home.string();
    }
}

ecf::cfg::Configuration ServerEnvironment::read_configuration(const ServerOptions& options,
                                                              const std::string& path_to_config_file) {
    const auto& catalogue = ecf::cfg::server_catalogue();
    auto variable = [](const char* name) { return ecf::cfg::Key::for_variable("ecflow.server.variables", name); };

    auto defaults = ecf::cfg::defaults_layer(catalogue);
    auto env_cfg  = ecf::cfg::CfgReader(catalogue).read(path_to_config_file);
    auto env      = ecf::cfg::EnvReader(catalogue).read();
    auto cli      = ecf::cfg::CliReader(catalogue).read(options.options());

    // The port given by the other origins names the configuration file; the host is that of the machine
    ecf::cfg::Configuration configuration({defaults, env_cfg, env, cli});
    const auto port = std::to_string(std::get<std::int64_t>(*configuration.get(variable("ECF_PORT"))));
    const auto file = host_name_.prefix_host_and_port(port, Ecf::CONFIG_FILE());
    // server_environment.cfg was inspected by its reader, which refuses any error other than its absence
    std::error_code env_cfg_error;
    const bool env_cfg_found = fs::exists(fs::status(path_to_config_file, env_cfg_error));

    // Only the absence of the file is accepted; any other error inspecting it stops start-up
    std::error_code ecf_cfg_error;
    const bool ecf_cfg_found = fs::exists(fs::symlink_status(file, ecf_cfg_error));
    if (ecf_cfg_error && ecf_cfg_error != std::errc::no_such_file_or_directory) {
        throw ecf::cfg::ConfigurationError("Configuration file '" + file +
                                           "' cannot be inspected: " + ecf_cfg_error.message());
    }

    // The paths are logged after the change to ECF_HOME, and so are made absolute
    const auto env_cfg_path = fs::absolute(path_to_config_file).lexically_normal().string();
    const auto ecf_cfg_path = fs::absolute(file).lexically_normal().string();
    configuration_sources_  = {{"defaults", "loaded"},
                               {"env.cfg (" + env_cfg_path + ")", env_cfg_found ? "loaded" : "absent"},
                               {"ecf.cfg (" + ecf_cfg_path + ")", ecf_cfg_found ? "loaded" : "absent"},
                               {"environment", "loaded"},
                               {"commandline", "loaded"}};

    // Each setting is traced to the source of its effective value; a switch is true when it differs from its default
    auto trace = [&](const ecf::cfg::Configuration& traced) {
        const std::map<std::string, std::string> origins{{std::string(ecf::cfg::layers::defaults), "defaults"},
                                                         {std::string(ecf::cfg::layers::env_cfg), "env.cfg"},
                                                         {std::string(ecf::cfg::layers::ecf_cfg), "ecf.cfg"},
                                                         {std::string(ecf::cfg::layers::env), "environment"},
                                                         {std::string(ecf::cfg::layers::cli), "commandline"}};
        configuration_origins_.clear();
        configuration_switches_.clear();
        for (const auto& setting : catalogue.settings()) {
            const auto& key         = setting.key();
            const auto source       = traced.source(key);
            const auto& switch_name = setting.names().cli;
            const auto name = key.is_variable() ? std::string(key.components().second) : "--" + switch_name.value();
            if (source) {
                configuration_origins_.emplace(name, origins.at(*source));
            }
            if (!key.is_variable()) {
                configuration_switches_.emplace_back(name, traced.get(key) != setting.default_value());
            }
        }
    };

    if (!ecf_cfg_found) {
        trace(configuration);
        return configuration;
    }

    auto ecf_cfg =
        ecf::cfg::Mapper(catalogue).map(ecf::cfg::JsonReader().read_file(file), ecf::cfg::layers::ecf_cfg, file);

    // The file belongs to the server of its name
    auto refuse = [&file](const char* name, const std::string& value, const char* part, const std::string& expected) {
        return ecf::cfg::ConfigurationError(file + ": /ecflow/server/variables/" + name + ": invalid value " + value +
                                            ": expected the " + part + " of the file name, " + expected);
    };
    if (const auto found = ecf_cfg.entries.find(variable("ECF_HOST")); found != ecf_cfg.entries.end()) {
        if (const auto& host = std::get<std::string>(found->second); host != host_name_.name()) {
            throw refuse("ECF_HOST", "'" + host + "'", "host", "'" + host_name_.name() + "'");
        }
    }
    if (const auto found = ecf_cfg.entries.find(variable("ECF_PORT")); found != ecf_cfg.entries.end()) {
        if (const auto value = std::to_string(std::get<std::int64_t>(found->second)); value != port) {
            throw refuse("ECF_PORT", value, "port", port);
        }
    }

    ecf::cfg::Configuration result(
        {std::move(defaults), std::move(env_cfg), std::move(ecf_cfg), std::move(env), std::move(cli)});
    trace(result);
    return result;
}

#ifdef ECF_OPENSSL
void ServerEnvironment::select_ssl(const ecf::cfg::Configuration& configuration) {
    const auto value  = configuration.get(ecf::cfg::Key::for_variable("ecflow.server.variables", "ECF_SSL"));
    const bool option = std::get<bool>(*configuration.get(ecf::cfg::Key::for_option("ecflow.server.ssl")));
    if (!value && !option) {
        return;
    }

    // ECF_SSL, when it has a value, decides the certificate lookup order, also when the option is set
    const auto request = value ? ecf::Openssl::Option::make_from_envvar_value(std::get<std::string>(*value))
                               : ecf::Openssl::Option::make_option_specific();
    if (debug_) {
        std::cout << "SSL enabled: " << request.describe(option) << '\n';
    }
    ssl_.enable(serverHost_, the_port(), request);
    update_protocol_after_ssl();
    if (debug_) {
        std::cout << ssl_.info() << '\n';
    }
}
#endif

void ServerEnvironment::change_dir_to_ecf_home_and_check_accesibility() {
    if (chdir(ecfHome_.c_str()) != 0) {
        throw ServerEnvironmentException(MESSAGE("Can't chdir to ECF_HOME " << ecfHome_ << "\n"));
    }
    if (access(ecfHome_.c_str(), X_OK | R_OK | W_OK) != 0) {
        // R_OK test for read permission
        // W_OK test for write permission
        // X_OK test for execute or search permission
        // F_OK  test whether the directories leading to the file can be searched and the file exists.
        throw ServerEnvironmentException(MESSAGE("Access restriction on ECF_HOME " << ecfHome_ << "\n"));
    }
}

std::string ServerEnvironment::check_mode_str() const {
    return the_check_mode(checkMode_);
}

std::string ServerEnvironment::dump() const {
    // The same content as the log; the authentication and authorisation, once loaded
    auto lines = runtime_configuration_lines();
    if (access_loaded_) {
        for (const auto& section : {authentication_lines(), authorisation_lines()}) {
            lines.insert(lines.end(), section.begin(), section.end());
        }
    }
    std::string text;
    for (const auto& line : lines) {
        text += line + "\n";
    }
    return text;
}

std::vector<std::string> ServerEnvironment::expected_variables() {
    std::vector<std::string> expected_variables;
    expected_variables.emplace_back(ecf::environment::ECF_HOME);
    expected_variables.emplace_back("ECF_LOG");
    expected_variables.emplace_back("ECF_CHECK");
    expected_variables.emplace_back("ECF_CHECKOLD");
    expected_variables.emplace_back("ECF_JOB_CMD");
    expected_variables.emplace_back("ECF_KILL_CMD");
    expected_variables.emplace_back("ECF_STATUS_CMD");
    expected_variables.emplace_back("ECF_CHECK_CMD");
    expected_variables.emplace_back("ECF_URL_CMD");
    expected_variables.emplace_back("ECF_URL_BASE");
    expected_variables.emplace_back("ECF_URL");
    expected_variables.emplace_back(ecf::environment::ECF_MICRO);
    expected_variables.emplace_back("ECF_PID");
    expected_variables.emplace_back("ECF_VERSION");
    expected_variables.emplace_back("ECF_LISTS");
    expected_variables.emplace_back(ecf::environment::ECF_PORT);
    expected_variables.emplace_back(ecf::environment::ECF_HOST);
    expected_variables.emplace_back("ECF_INTERVAL");
    expected_variables.emplace_back(ecf::environment::ECF_PASSWD);
    expected_variables.emplace_back(ecf::environment::ECF_CUSTOM_PASSWD);
#ifdef ECF_OPENSSL
    if (ecf::environment::has(ecf::environment::ECF_SSL)) {
        expected_variables.emplace_back(ecf::environment::ECF_SSL);
    }
#endif
    return expected_variables;
}
