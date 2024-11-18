/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_ServerEnvironment_HPP
#define ecflow_server_ServerEnvironment_HPP

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

#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PasswdFile.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif

// Added ServerEvinronmentException so that it can be in the same scope as server
// in ServerMain. Previously we had a separate try block
// Also distinguish between server and environment errors
class ServerEnvironmentException : public std::runtime_error {
public:
    explicit ServerEnvironmentException(const std::string& msg) : std::runtime_error(msg) {}
};

class ServerEnvironment {
public:
    ServerEnvironment(const CommandLine& cl, const std::string& path_to_config_file = "server_environment.cfg");

    ServerEnvironment(int argc, char* argv[]);
    ServerEnvironment(int argc, char* argv[], const std::string& path_to_config_file);

    // Disable copy (and move) semantics
    ServerEnvironment(const ServerEnvironment&)                  = delete;
    const ServerEnvironment& operator=(const ServerEnvironment&) = delete;

    ~ServerEnvironment();

    /// return true if option are valid false and error message otherwise
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
    void enable_ssl() { ssl_.enable(serverHost_, the_port()); } // search server.crt first, then <host>.<port>.crt
#endif

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
        if (v > 0)
            checkPtInterval_ = v;
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

    bool reloadPasswdFile(std::string& errorMsg);
    bool reloadCustomPasswdFile(std::string& errorMsg);

    /// There are several kinds of authentification:
    ///     a/ None
    ///     b/ List mode.   ASCII file based on ECF_LISTS is defined
    ///     c/ Secure mode. binary file based ECF_PASSWD is defined
    /// At the moment we will only implement options a/ and b/
    //
    /// Returns true if the given user has access to the server, false otherwise
    bool authenticateReadAccess(const std::string& user, bool custom_user, const std::string& passwd) const;
    bool authenticateReadAccess(const std::string& user,
                                bool custom_user,
                                const std::string& passwd,
                                const std::string& path) const;
    bool authenticateReadAccess(const std::string& user,
                                bool custom_user,
                                const std::string& passwd,
                                const std::vector<std::string>& paths) const;
    bool authenticateWriteAccess(const std::string& user) const;
    bool authenticateWriteAccess(const std::string& user, const std::string& path) const;
    bool authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths) const;

    /// return true if help option was selected
    bool help_option() const { return help_option_; }
    bool version_option() const { return version_option_; }

    /// debug is enabled via --debug or server invocation
    bool debug() const { return debug_; }
    void set_debug(bool f) { debug_ = f; }

    std::string dump() const;

    static std::vector<std::string> expected_variables();

private:
    void init(const CommandLine& cl, const std::string& path_to_config_file);

    ///  defaults are read from a config file
    void read_config_file(std::string& log_file_name, const std::string& path_to_config_file);

    /// Get the standard environment variables, overwrite any settings from config file
    void read_environment_variables(std::string& log_file_name);

    void change_dir_to_ecf_home_and_check_accesibility();

    bool load_whitelist_file(std::string& err) const;

private:
    ecf::Host host_name_;
    std::string serverHost_; // must be after host_name_, since used in init
    int serverPort_;
    int checkPtInterval_;
    int checkpt_save_time_alarm_;
    int submitJobsInterval_;
    int ecf_prune_node_log_;
    bool jobGeneration_; // used in debug/test mode only
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

    std::string ecf_passwd_file_;
    std::string ecf_passwd_custom_file_;
    mutable PasswdFile passwd_file_;
    mutable PasswdFile passwd_custom_file_;

#ifdef ECF_OPENSSL
    ecf::Openssl ssl_;
#endif

    boost::asio::ip::tcp tcp_protocol_; // defaults to IPv4 TCP protocol
    friend class ServerOptions;
};

#endif /* ecflow_server_ServerEnvironment_HPP */
