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

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/asio.hpp>

#include "ecflow/base/ServerProtocol.hpp"
#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/core/cfg/Configuration.hpp"
#include "ecflow/server/AuthenticationService.hpp"
#include "ecflow/server/AuthorisationService.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif

// Added ServerEvinronmentException so that it can be in the same scope as server
// in ServerMain. Previously we had a separate try block
// Also distinguish between server and environment errors
class ServerOptions;

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
    /// The settings are checked by the server catalogue at construction; this checks the server host, the log and
    /// the security files.
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

    ///
    /// @brief Reloads the startup-selected whitelist while preserving active permissions on failure.
    ///
    /// Behaviour:
    /// - Reuses the startup-selected ECF_LISTS path without rereading configuration.
    /// - Updates live-server read/write permissions after successful parsing.
    /// - Accepts empty and version-only whitelists without whitelist restrictions.
    /// - Leaves password authentication unchanged.
    ///
    /// Reload fails if the selected file is:
    /// - Missing.
    /// - Unreadable.
    /// - Not a regular file.
    /// - Malformed.
    ///
    /// Failed reloads leave previously active permissions unchanged.
    ///
    /// @param[in,out] errorMsg Receives appended path and cause diagnostics on failure
    /// @return True if replacement permissions were parsed and installed successfully.
    /// @throws std::filesystem::filesystem_error if filesystem inspection fails.
    ///
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

    ///
    /// @brief Describes the server environment, as written to the log.
    ///
    /// @return The runtime configuration, followed by the authentication and the authorisation once loaded by valid().
    ///
    std::string dump() const;

    ///
    /// @brief Logs the authentication in force: the password files, and the users they list.
    ///
    /// The same lines are written to the standard output when debugging.
    ///
    void log_authentication() const;

    static std::vector<std::string> expected_variables();

private:
    ///
    /// @brief Selects the startup settings from the built-in defaults, the configuration file, the environment and
    /// the command line.
    ///
    /// Command-line syntax is parsed first. Help and version requests return without reading configuration
    /// or environment settings, applying transport options or accessing runtime files. Each value is checked by
    /// the server catalogue when it is read.
    ///
    /// @param[in] cl Command-line tokens, overriding configuration and environment settings
    /// @param[in] path_to_config_file Optional configuration path resolved before changing directory
    /// @throws ServerEnvironmentException if reading a setting or server-home access fails.
    /// @throws std::exception if command-line parsing or runtime file setup fails.
    ///
    void init(const CommandLine& cl, const std::string& path_to_config_file);

    ///
    /// @brief Takes the settings from their effective values, and resolves a relative server home.
    ///
    /// A relative server home becomes an absolute path against the startup directory, in normal form, without dot
    /// segments or a trailing separator. An absolute server home is preserved.
    ///
    /// @param[in] configuration The configuration of the server, every value of which is valid
    /// @param[out] log_file_name Receives the log filename
    /// @throws std::filesystem::filesystem_error if the current directory cannot be determined.
    ///
    void resolve(const ecf::cfg::Configuration& configuration, std::string& log_file_name);

    ///
    /// @brief Reads the configuration of the server from all its origins.
    ///
    /// The port given by the built-in defaults, `server_environment.cfg`, the environment and the command line names
    /// the configuration file, `<host>.<port>.ecf.cfg`, looked for in the current directory. When the file exists,
    /// its layer is placed above `server_environment.cfg`, and below the environment.
    ///
    /// The sources, each loaded or absent, the origin of the effective value of each setting, and the value of each
    /// switch are recorded, to be logged once the log exists.
    ///
    /// @param[in] options The parsed command line
    /// @param[in] path_to_config_file The path of `server_environment.cfg`
    /// @return The configuration, every value of which is valid.
    /// @throws ecf::cfg::ConfigurationError if an origin is refused, or the configuration file defines a host or a
    /// port other than those of its name.
    ///
    ecf::cfg::Configuration read_configuration(const ServerOptions& options, const std::string& path_to_config_file);

#ifdef ECF_OPENSSL
    ///
    /// @brief Enables SSL when ECF_SSL has a value or the SSL option is set, and promotes the protocol accordingly.
    ///
    /// The certificate lookup order is the following:
    ///
    ///   - ECF_SSL empty or "1":   `server.crt` first, then `<host>.<port>.crt`;
    ///   - ECF_SSL with any other value:   `<host>.<port>.crt` first, then `server.crt`;
    ///   - the SSL option alone:   as ECF_SSL with any other value.
    ///
    /// @param[in] configuration The configuration of the server, every value of which is valid
    /// @throws std::runtime_error if SSL is requested but neither certificate file exists.
    ///
    void select_ssl(const ecf::cfg::Configuration& configuration);
#endif

    void change_dir_to_ecf_home_and_check_accesibility();

    ///
    /// @brief Loads a regular whitelist file and installs its rules only after successful parsing.
    ///
    /// Failed parsing leaves the previously active permissions unchanged.
    ///
    /// @param[in,out] err Receives appended diagnostics identifying the selected path and cause on failure
    /// @return True if the file was loaded and its authorisation rules were installed.
    /// @throws std::filesystem::filesystem_error if filesystem inspection fails.
    ///
    bool load_whitelist_file(std::string& err) const;

    ///
    /// @brief Logs the authorisation in force: its mode, the whitelist file, and the access of each user.
    ///
    /// The mode is `whitelist` once a whitelist file is loaded, and `nodes` otherwise, i.e. the permissions of the
    /// nodes apply.
    ///
    void log_authorisation() const;

    ///
    /// @brief Returns the runtime configuration: the sources read, and each value with its source.
    ///
    /// @return The lines, starting with the title `Runtime Configuration:`.
    ///
    [[nodiscard]] std::vector<std::string> runtime_configuration_lines() const;

    ///
    /// @brief Returns the authentication in force: the password files, and the users they list.
    ///
    /// @return The lines, starting with the title `Authentication:`.
    ///
    [[nodiscard]] std::vector<std::string> authentication_lines() const;

    ///
    /// @brief Returns the authorisation in force: its mode, the whitelist file, and the access of each user.
    ///
    /// @return The lines, starting with the title `Authorisation:`.
    ///
    [[nodiscard]] std::vector<std::string> authorisation_lines() const;

    ///
    /// @brief Writes lines to the log, and to the standard output when debugging.
    ///
    /// @param[in] lines The lines
    ///
    void report(const std::vector<std::string>& lines) const;

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
    std::vector<std::pair<std::string, std::string>> configuration_sources_; ///< Source, "loaded" or "absent"
    std::map<std::string, std::string> configuration_origins_;               ///< Origin of each value, by name
    std::vector<std::pair<std::string, bool>> configuration_switches_;       ///< Value of each switch, e.g. --http
    mutable WhiteListFile white_list_file_;

    mutable bool access_loaded_{false}; ///< Whether valid() loaded the authentication and authorisation
    mutable ecf::AuthenticationService authentication_service_;
    mutable ecf::AuthorisationService authorisation_service_;

#ifdef ECF_OPENSSL
    ecf::Openssl ssl_;
#endif

    boost::asio::ip::tcp tcp_protocol_; // defaults to IPv4 TCP protocol
};
