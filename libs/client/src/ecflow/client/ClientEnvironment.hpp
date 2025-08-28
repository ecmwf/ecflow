/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_ClientEnvironment_HPP
#define ecflow_client_ClientEnvironment_HPP

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/base/AbstractClientEnv.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif
#include "ecflow/base/ServerProtocol.hpp"

/**
 * @brief The ClientEnvironment is used to manage the client configuration loaded from the execution environment.
 *
 * The ClientEnvironment is responsible for loading the client related environment variables.
 * These variables are extracted from several sources:
 *  - the execution environment (e.g. ECF_NAME, ECF_PASS, etc.),
 *  - the host file (ECF_HOSTFILE)
 *
 * The ClientEnvironment can be configured with multiple hosts (host/port), provided either explicitly or via
 * ECF_HOST/ECF_PORT plus a list of host/port pairs in the ECF_HOSTFILE. This list of hosts is iterated over
 * to enable the client to connect to alternative servers, retrying when the 'principal' host connection fails.
 * The content of the ECF_HOSTFILE is lazy-loaded (i.e. it is only loaded when the connection fails and an alternate
 * host:port is required).
 */
class ClientEnvironment final : public AbstractClientEnv {
public:
    /**
     * @brief Create a new ClientEnvironment, effectively loading all environment configurations
     *
     * @param gui If true, the environment is set up for a GUI client.
     */
    explicit ClientEnvironment(bool gui);

    /**
     * @brief Create a new ClientEnvironment, effectiveling loads all environment configurations
     *
     * @param gui If true, the environment is set up for a GUI client.
     * @param host The host to be contacted by the client
     * @param port The port to be contacted by the client
     */
    explicit ClientEnvironment(bool gui, const std::string& host, const std::string& port);

    /**
     * @brief Create a new ClientEnvironment, effectively loads all environment configurations
     *
     * @remark This constructor is available for testing purposes only.
     *
     * @param hostFile The path to a file containing a list of hosts and ports
     * @param host The host to be contacted by the client
     * @param port The port to be contacted by the client
     */
    explicit ClientEnvironment(const std::string& hostFile, const std::string& host = "", const std::string& port = "");

    /**
     * @brief Access the value of Task (or Child) command timeout.
     *
     * The `Task command timeout` is the maximum time in seconds for the client to deliver the message.
     * The client will continue to attempt to contact the server until this timeout is reached.
     *
     * The `Task command timeout` default is 24 hours
     * but can be customised by exporting environment variable ECF_TIMEOUT.
     *
     * @remark  This timeout is currently used for the Task commands *ONLY*
     */
    long max_child_cmd_timeout() const { return timeout_; }

    /**
     * @brief Set the value of Task (or Child) command timeout.
     *
     * @remark Intended to be used only by the Python API
     */
    void set_child_cmd_timeout(unsigned int t) { timeout_ = t; }

    /**
     * @brief Access the value of 'Zombie' Task (or Child) command timeout.
     *
     * The `Zombie Task command timeout` is the maximum time in seconds for the client to deliver the message.
     * The client, running on a Zombie task, will continue to attempt to contact the server until this timeout is
     * reached.
     *
     * The `Zombie command timeout` default is 12 hours
     * but can be customised by exporting environment variable ECF_ZOMBIE_TIMEOUT.
     *
     * @remark  This timeout is currently used for the Zombie commands *ONLY*
     */
    long max_zombie_child_cmd_timeout() const { return zombie_timeout_; }

    /**
     * @brief Set the value of 'Zombie' Task (or Child) command timeout.
     *
     * @remark Intended to be used only by the Python API
     */
    void set_zombie_child_cmd_timeout(unsigned int t) { zombie_timeout_ = t; }

    /// The timeout feature allow the client to fail gracefully in the case
    /// where the server has died/crashed. The timeout will ensure the socket is closed.
    /// allowing the server to be restarted without getting the address is use error.
    /// Set the timeout for each client->server communication:
    //    connect        : timeout_ second
    //    send request   : timeout_ second
    //    receive reply  : timeout_ second
    // default is 0 second, which means take the timeout from the command/request
    // The default can be overridden with ECF_CONNECT_TIMEOUT.
    // This is useful when running valgrind --tool=callgrind which leads to
    // massive slow down and disconnection from the client, when the server is still
    // running.
    // used for **test only**
    int connect_timeout() const { return connect_timeout_; }

    /// When called will demand load the ecf host file, and read the hosts.
    /// Will then iterate over the host, when the end is reached will start over
    /// If host cannot be opened or errors occur returns false;
    bool get_next_host(std::string& errorMsg);

    /// If server denies client communication and this flag is set, exit with an error
    /// Avoids 24hr hour connection attempt to server. Aids zombie control.
    bool denied() const { return denied_; }

    /// if NO_ECF is set then abort client immediately.  Client should return success
    /// useful when we want to test jobs stand-alone
    bool no_ecf() const { return no_ecf_; }

    /// for debug
    std::string toString() const;

    /// Return contents of ECF_HOST environment variable, Otherwise an EMPTY string
    static std::string hostSpecified();

    /// Returns of ECF_PORT,environment variable, otherwise returns Str::DEFAULT_PORT_NUMBER
    static std::string portSpecified();

    // Set the rid. Only needed to override in regression test, to avoid interference
    // Should use a better name, as this is really the Process identifier of the running job.
    // Needs to be the same value as supplied to child command init
    void set_remote_id(const std::string& rid) { remote_id_ = rid; }

    // Used by python to enable debug of client api
    void set_debug(bool flag);

    ecf::Protocol protocol() const { return protocol_; }
    void enable_http() { protocol_ = ecf::Protocol::Http; }
    void enable_https() { protocol_ = ecf::Protocol::Https; }

#ifdef ECF_OPENSSL
    /// return true if this is a ssl enabled server
    ecf::Openssl& openssl() { return ssl_; }
    const ecf::Openssl& openssl() const { return ssl_; }
    bool ssl() const { return ssl_.enabled(); }
    void enable_ssl_if_defined() {
        protocol_ = ecf::Protocol::Ssl;
        ssl_.enable_if_defined(host(), port());
    } // IF ECF_SSL=1,search server.crt, ELSE search <host>.<port>.crt
    void enable_ssl() {
        protocol_ = ecf::Protocol::Ssl;
        ssl_.enable(host(), port());
    } // search server.crt first, then <host>.<port>.crt
    bool enable_ssl_no_throw() {
        protocol_ = ecf::Protocol::Ssl;
        return ssl_.enable_no_throw(host(), port());
    } // search server.crt first, then <host>.<port>.crt
    void disable_ssl() {
        protocol_ = ecf::Protocol::Plain;
        ssl_.disable();
    } // override environment setting for ECF_SSL
#endif

    /**
     * @see AbstractClientEnv::set_cli()
     */
    void set_cli(bool f) override { cli_ = f; }
    bool get_cli() const override { return cli_; }
    bool checkTaskPathAndPassword(std::string& errorMsg) const override;
    const std::string& task_path() const override { return task_path_; }
    int task_try_no() const override { return task_try_num_; }
    const std::string& jobs_password() const override { return jobs_password_; }
    const std::string& process_or_remote_id() const override { return remote_id_; }

    /**
     * @brief Set the host and port for the client.
     *
     * @remark Explicitly setting the host/port disables loading the host file, and thus the connection retry mechanism.
     *
     * @param host the host to be contacted by the client
     * @param port the port to be contacted by the client
     */
    void set_host_port(const std::string& host, const std::string& port) override;
    const std::string& host() const override;
    const std::string& port() const override;
    const std::vector<std::pair<std::string, std::string>>& env() const override { return env_; }
    const std::string& get_user_password(const std::string& user) const override;
    const std::string& get_custom_user_password(const std::string& user) const override;
    void clear_user_password() override { passwd_.clear(); }
    const std::string& get_user_name() const override { return user_name_; }
    void set_user_name(const std::string& name) override { user_name_ = name; }
    void set_password(const std::string& password) override { passwd_ = password; }
    bool debug() const override { return debug_; } // enabled if ECF_DEBUG_CLIENT set
    void set_test() override { under_test_ = true; }
    bool under_test() const override { return under_test_; }

    // Support for python child commands, and python jobs
    void set_child_path(const std::string& path) { task_path_ = path; }
    void set_child_password(const std::string& pass) { jobs_password_ = pass; }
    void set_child_pid(const std::string& pid) { remote_id_ = pid; }
    void set_child_try_no(unsigned int try_no) { task_try_num_ = try_no; }
    void set_child_init_add_vars(const std::vector<Variable>& vars) { init_add_vars_ = vars; }
    void set_child_complete_del_vars(const std::vector<std::string>& vars) { complete_del_vars_ = vars; }
    void set_child_host_file(const std::string& host_file) { host_file_ = host_file; }
    void set_child_denied(bool denied) { denied_ = denied; }
    void set_child_no_ecf(bool no_ecf) { no_ecf_ = no_ecf; }

    [[nodiscard]] const std::string& host_file() const { return host_file_; }

    const std::vector<Variable>& init_add_vars() const { return init_add_vars_; }
    const std::vector<std::string>& complete_del_vars() const { return complete_del_vars_; }

    bool host_file_policy_is_task() const;
    bool host_file_policy_is_all() const;

private:
    std::string task_path_;        // ECF_NAME = /aSuite/aFamily/aTask
    std::string jobs_password_;    // ECF_PASS jobs password
    std::string remote_id_;        // ECF_RID process id of the running job
    std::string host_file_;        // ECF_HOSTFILE. File that lists the backup hosts, port numbers must match
    std::string host_file_policy_; // ECF_HOSTFILE_POLICY. The policy for using the host file ("all" or "task")
    std::string user_name_;
    mutable std::string passwd_;

    long timeout_;        // ECF_TIMEOUT. Host file iteration time out
    long zombie_timeout_; // ECF_ZOMBIE_TIMEOUT. Host file iteration time out for zombies, default same as ECF_TIMEOUT
    std::vector<Variable> init_add_vars_;
    std::vector<std::string> complete_del_vars_;

    std::vector<std::pair<std::string, std::string>> env_;      // For test allow env variable to be set on defs
    std::vector<std::pair<std::string, std::string>> host_vec_; // The list of host:port pairs

#ifdef ECF_OPENSSL
    ecf::Openssl ssl_;
#endif

    /**
     * @brief The number of times the Task execution has been attempted, based on ECF_TRYNO environment variable.
     *
     * This value starts at 1, and is incremented each time the Task is retried.
     */
    unsigned int task_try_num_{1}; // ECF_TRYNO. The task try number. The number of times the job has been run

    /**
     * @brief The connection timeout (in seconds), based on ECF_CONNECT_TIMEOUT environment variable.
     *
     * This value default is 0.
     */
    int connect_timeout_{0}; // default 0, ECF_CONNECT_TIMEOUT, connection timeout

    /**
     * @brief The index of the current selected host, used to select an entry into the host_vec_ vector.
     */
    int host_vec_index_{0}; // index into host_vec;

    bool cli_{false};    // Command Line Interface
    bool denied_{false}; // ECF_DENIED.If the server denies the communication, then the child command can be set to fail
                         // immediately
    bool no_ecf_{false}; // NO_ECF. if defined then abort cmd immediately. useful when test jobs stand-alone
    ecf::Protocol protocol_{ecf::Protocol::Plain};
    bool debug_{false};          // For live debug, enabled by env variable ECF_CLIENT_DEBUG or set by option -d|--debug
    bool under_test_{false};     // Used in testing client interface
    bool host_file_read_{false}; // to ensure we read host file only once
    bool gui_{false};

    /// The option read from the command line.
    friend class ClientOptions;

    // Allow testing to override the task path set in the environment. In this constructor
    friend class ClientInvoker;

private:
    void init();

    void read_environment_variables(); /// Get the standard environment variables

    void taskPath(const std::string& s) { task_path_ = s; }
    void set_jobs_password(const std::string& s) { jobs_password_ = s; }
    void set_connect_timeout(int t) { connect_timeout_ = t; }

    // Allow testing to add or update the environment in the Defs file
    // Each pair is ( variable name, variable value )
    void setEnv(const std::vector<std::pair<std::string, std::string>>& e) { env_ = e; }

    bool parseHostsFile(std::string& errorMsg);

    const std::string& get_password(const char*, const std::string& user) const;
};

#endif /* ecflow_client_ClientEnvironment_HPP */
