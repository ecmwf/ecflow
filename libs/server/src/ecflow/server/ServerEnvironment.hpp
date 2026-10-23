// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

///
/// \brief The server environment is read from the configuration file. This defines
/// the defaults for the server,and is configurable by the user, at run time
/// Alternatively some variable can be set via environment. See environment.cfg
///
/// The server must be created before the log file. Since the log is initialised
/// with the log file name from the server environment
///

#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "ecflow/base/ServerProtocol.hpp"
#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/server/AuthenticationService.hpp"
#include "ecflow/server/AuthorisationService.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif

// Added ServerEvinronmentException so that it can be in the same scope as server
// in ServerMain. Previously we had a separate try block
// Also distinguish between server and environment errors
class ServerEnvironmentException : public std::runtime_error {
public:
    explicit ServerEnvironmentException(const std::string& msg)
        : std::runtime_error(msg) {}
};

class ServerEnvironment {
public:
    explicit ServerEnvironment(const CommandLine& cl,
                               const std::string& path_to_config_file = "server_environment.cfg");

    explicit ServerEnvironment(const std::vector<std::string>& args)
        : ServerEnvironment(CommandLine(args)) {}
    explicit ServerEnvironment(const std::vector<std::string>& args, const std::string& path_to_config_file)
        : ServerEnvironment(CommandLine(args), path_to_config_file) {}

    // Disable copy (and move) semantics
    ServerEnvironment(const ServerEnvironment&)                  = delete;
    const ServerEnvironment& operator=(const ServerEnvironment&) = delete;
    ServerEnvironment(ServerEnvironment&&)                       = delete;
    ServerEnvironment& operator=(ServerEnvironment&&)            = delete;

    ~ServerEnvironment();

    ///
    /// @brief Validates the effective server configuration and loads optional security files.
    ///
    /// The checkpoint interval must be positive after configuration and environment overrides have been applied.
    ///
    /// @param[in,out] errorMsg Receives diagnostic details for an invalid configuration or security file
    /// @return True if the configuration and optional security files are valid; false otherwise.
    /// @throws std::filesystem::filesystem_error if a security-file path cannot be inspected.
    ///
    bool valid(std::string& errorMsg) const;

    /// return the directory path associated with smshome
    /// but can be overridden by the environment variable ECF_HOME
    std::string ecf_home() const { return ecfHome_; }

    /// returns the server host and port
    std::pair<std::string, std::string> hostPort() const;

#ifdef ECF_OPENSSL
    /// return true if ssl enable via command line, AND SSL libraries are found
    ecf::Openssl& openssl() { return ssl_; }
    const ecf::Openssl& openssl() const { return ssl_; }
    bool ssl() const { return ssl_.enabled(); }

    ///
    /// @brief Enables SSL according to ECF_SSL when the variable is set.
    ///
    /// Empty or "1" searches `server.crt` first, then `<host>.<port>.crt`;
    /// any other value searches `<host>.<port>.crt` first, then `server.crt`.
    ///
    /// @throws std::runtime_error if ECF_SSL is set but neither certificate file exists.
    ///
    void enable_ssl_if_defined() {
        ssl_.enable_based_on_envvar(serverHost_, the_port());
        update_protocol_after_ssl();
    }

    ///
    /// @brief Enables SSL preferring the shared certificate, independently of ECF_SSL.
    ///
    /// @throws std::runtime_error if neither the shared nor the server-specific certificate file exists.
    ///
    void enable_ssl() {
        ssl_.enable_prefer_shared(serverHost_, the_port());
        update_protocol_after_ssl();
    }
#endif

    ecf::Protocol protocol() const { return protocol_; }

    /// returns the server port. This has a default value defined in server_environment.cfg
    /// but can be overridden by the environment variable ECF_PORT
    int port() const { return serverPort_; }
    std::string the_port() const;

    /// returns the TCP protocol. default is TCPv4. Can be changed via command line to TCPv6
    boost::asio::ip::tcp tcp_protocol() const { return tcp_protocol_; }

    /// Return true if job generation is enabled. By default job generation is enabled however
    /// for test/debug we can elect to disable, it.
    bool jobGeneration() const { return jobGeneration_; }

    /// Whenever we save the checkpt, we time how long this takes.
    /// For very large definition the time can be significant and start to interfere with
    /// the scheduling. (i.e since write to disk is blocking).
    /// The default is defined by CheckPt::default_save_time_alarm(). When the save time exceeds this, the late flag is
    /// set on the definition. This needs to be manually reset.
    size_t checkpt_save_time_alarm() const { return checkpt_save_time_alarm_; }
    void set_checkpt_save_time_alarm(size_t t) { checkpt_save_time_alarm_ = t; }

    /// returns the path of the checkpoint file. This has a default name set in server_environment.cfg
    /// but can be overridden by the environment variable ECF_CHECK
    /// if the check point file starts with an absolute path return as is:
    /// otherwise returns ECF_HOME/ecf_checkpt_file_
    const std::string& checkPtFilename() const { return ecf_checkpt_file_; }

    /// returns the path of the old checkPointFile. This has a default name set in server_environment.cfg
    /// but can be overridden by the environment variable ECF_CHECKOLD
    /// if the check point file starts with an absolute path return as is:
    /// otherwise returns ECF_HOME/ecf_backup_checkpt_file_
    const std::string& oldCheckPtFilename() const { return ecf_backup_checkpt_file_; }

    /// returns the checkPt interval. This is the time in seconds, at which point the server
    /// serializes the defs node tree. This is called the check point file.
    /// This has a default value set in environment.cfg
    /// but can be overridden by the environment variable ECF_CHECKINTERVAL
    /// if set via environment variable it must be convertible to an integer
    int checkPtInterval() const { return checkPtInterval_; }

    /// set the check point interval. Typically set via client interface
    /// value should > 0 for valid values.
    void set_checkpt_interval(int v) {
        if (v > 0) {
            checkPtInterval_ = v;
        }
    }

    /// returns the check mode. This specifies options for saving of the checkPoint file:
    ecf::CheckPt::Mode checkMode() const { return checkMode_; }
    void set_check_mode(ecf::CheckPt::Mode m) { checkMode_ = m; }
    std::string check_mode_str() const;

    /// returns the number of seconds at which we should check time dependencies
    /// this includes evaluating trigger dependencies and submit the corresponding jobs.
    /// This is set at 60 seconds. But will vary for debug/test purposes only.
    /// For Testing the state change queued->submitted->active duration < submitJobsInterval
    /// If this state change happens at the job submission boundary then
    /// time series can get a skew.
    int submitJobsInterval() const { return submitJobsInterval_; }

    /// Returns ECF_PRUNE_NODE_LOG as an integer representing days
    /// Node log/edit history older than this is pruned when loading the checkpoint file
    /// Default value is 30 days. To alter change config file or set an environment variable.
    /// A value of 0, means no pruning. i.e keep old edit history .
    int ecf_prune_node_log() const { return ecf_prune_node_log_; }

    /// returns server variables, as vector of pairs.
    /// Some of these variables hold environment variables
    /// Note:: additional variable are created for use by clients, i.e like
    ///        ECF_PORT(jobs, server, client)
    ///        ECF_HOST(jobs).   ECF_HOST is the server machine host name
    void variables(std::vector<std::pair<std::string, std::string>>&) const;

    /// Ask the server to reload file the hold list of users and their access rights
    /// The white list file is specified by the environment variable ECF_LISTS
    /// This allows/disallows user access on the live server
    /// Return true if file is reloaded ok, else false and error message if:
    /// 	a/ File does not exist
    ///  	b/ File is empty
    ///  	c/ Errors in parsing file
    /// If errors arise the exist user still stay in affect
    bool reloadWhiteListFile(std::string& errorMsg);

    ecf::AuthenticationService& authentication() { return authentication_service_; }
    const ecf::AuthenticationService& authentication() const { return authentication_service_; }

    ecf::AuthorisationService& authorisation() { return authorisation_service_; }
    const ecf::AuthorisationService& authorisation() const { return authorisation_service_; }

    // const PasswdFile& passwd_file() const { return passwd_file_; }
    // const PasswdFile& passwd_custom_file() const { return passwd_custom_file_; }

    /// return true if help option was selected
    bool help_option() const { return help_option_; }
    bool version_option() const { return version_option_; }

    /// debug is enabled via --debug or server invocation
    bool debug() const { return debug_; }
    void set_debug(bool f) { debug_ = f; }

    std::string dump() const;

    static std::vector<std::string> expected_variables();

private:
    ///
    /// @brief Selects startup settings and validates required strings and the effective profiling threshold.
    ///
    /// @param[in] cl Command-line tokens, applied after configuration and environment settings
    /// @param[in] path_to_config_file Optional configuration path resolved before changing directory
    /// @throws ServerEnvironmentException if parsing, required-setting validation or server-home access fails.
    /// @throws std::exception if command-line parsing or runtime file setup fails.
    ///
    void init(const CommandLine& cl, const std::string& path_to_config_file);

    ///
    /// @brief Reads configuration settings and applies built-in defaults for unspecified options.
    ///
    /// A missing or empty configuration file uses the built-in defaults. An existing file must be readable
    /// and regular. File-access and parsing failures stop initialisation
    /// before environment overrides or startup side effects are applied.
    /// An explicitly specified checkpoint mode must match one of the supported names.
    ///
    /// @param[in,out] log_file_name Receives the configured log filename or its built-in default
    /// @param[in] path_to_config_file Configuration path, relative to the startup directory or absolute
    /// @throws ServerEnvironmentException if file access, parsing or checkpoint-mode validation fails, including the
    /// path and cause.
    ///
    void read_config_file(std::string& log_file_name, const std::string& path_to_config_file);

    ///
    /// @brief Applies environment overrides and resolves a relative server home against the startup directory.
    ///
    /// Absolute server-home paths are preserved. An empty value is left for startup validation to reject.
    ///
    /// @param[in,out] log_file_name Configured log filename, replaced when ECF_LOG is set in the environment
    /// @throws ServerEnvironmentException if a numeric environment setting cannot be parsed.
    /// @throws std::filesystem::filesystem_error if the current directory cannot be determined.
    ///
    void read_environment_variables(std::string& log_file_name);

    void change_dir_to_ecf_home_and_check_accesibility();

    bool load_whitelist_file(std::string& err) const;

#ifdef ECF_OPENSSL
    ///
    /// @brief Promotes the protocol to reflect that SSL has been enabled.
    ///
    /// The underlying transport is preserved: HTTP yields HTTPS, and the custom TCP/IP protocol yields SSL.
    /// No promotion occurs when SSL was not actually enabled, for example when no certificate was found.
    ///
    void update_protocol_after_ssl() {
        if (!ssl_.enabled()) {
            return;
        }
        protocol_ = (protocol_ == ecf::Protocol::Http) ? ecf::Protocol::Https : ecf::Protocol::Ssl;
    }
#endif

private:
    ecf::Host host_name_;
    std::string serverHost_; // must be after host_name_, since used in init
    int serverPort_;
    int checkPtInterval_;
    int checkpt_save_time_alarm_;
    int submitJobsInterval_;
    int ecf_prune_node_log_;
    int ecf_task_threshold_; ///< Effective signed millisecond threshold, validated before updating the profiler.
    bool jobGeneration_;     // used in debug/test mode only
    ecf::Protocol protocol_;
    bool debug_;
    bool help_option_;
    bool version_option_;
    ecf::CheckPt::Mode checkMode_;
    std::string ecfHome_;
    std::string ecf_checkpt_file_;
    std::string ecf_backup_checkpt_file_;
    std::string ecf_pid_;
    std::string killCmd_;
    std::string statusCmd_;
    std::string checkCmd_;
    std::string urlCmd_;
    std::string urlBase_;
    std::string url_;
    std::string ecf_cmd_;
    std::string ecf_micro_;
    std::string ecf_white_list_file_;
    mutable WhiteListFile white_list_file_;

    mutable ecf::AuthenticationService authentication_service_;
    mutable ecf::AuthorisationService authorisation_service_;

#ifdef ECF_OPENSSL
    ecf::Openssl ssl_;
#endif

    boost::asio::ip::tcp tcp_protocol_; // defaults to IPv4 TCP protocol
    friend class ServerOptions;
};
