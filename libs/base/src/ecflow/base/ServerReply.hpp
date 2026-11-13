// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <variant>

#include "ecflow/attribute/Zombie.hpp"
#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/core/ErrorCode.hpp"
#include "ecflow/node/NodeFwd.hpp"

///
/// @brief Retains the most recent server reply and the client state that persists between requests.
///
/// The client handle and cached definitions persist; numeric error provenance and grouped failures do not.
///

class ServerReply {
public:
    enum News_t { NO_NEWS, NEWS, DO_FULL_SYNC };

    ServerReply() = default;

    ServerReply(const ServerReply&)            = delete;
    ServerReply& operator=(const ServerReply&) = delete;
    ServerReply(ServerReply&&)                 = delete;
    ServerReply& operator=(ServerReply&&)      = delete;

    ~ServerReply() = default;

    ///
    /// @brief Clears per-request state while retaining the client handle and cached definitions.
    ///
    /// Numeric error presence, raw message and grouped entries are discarded before the next request.
    ///
    /// @param[in] command_line_interface Whether the next request originates from the command line.
    ///
    void clear_for_invoke(bool command_line_interface); // true if calling from command line

    /// task based replies
    bool block_client_on_home_server() const { return block_client_on_home_server_; }
    void set_block_client_on_home_server() { block_client_on_home_server_ = true; }

    bool block_client_server_halted() const { return block_client_server_halted_; }
    void set_block_client_server_halted() { block_client_server_halted_ = true; }

    bool block_client_zombie_detected() const { return block_client_zombie_detected_; }
    void set_block_client_zombie_detected() { block_client_zombie_detected_ = true; }

    bool invalid_argument() const { return invalid_argument_; }
    void set_invalid_argument() { invalid_argument_ = true; }

    bool eof() const { return eof_; }
    void set_eof() { eof_ = true; }

    ///
    /// @brief Provides the structured diagnosis of the most recent exchange.
    ///
    /// The diagnosis is populated by the transport that performed the exchange. It records "no
    /// failure" whenever the exchange succeeded, and whenever the transport predates this
    /// mechanism.
    ///
    /// @return The diagnosis of the most recent exchange
    ///
    const ecf::ConnectionDiagnosis& diagnosis() const { return diagnosis_; }

    ///
    /// @brief Records the structured diagnosis of the most recent exchange.
    ///
    /// @param[in] diagnosis The diagnosis observed by the transport
    ///
    void set_diagnosis(const ecf::ConnectionDiagnosis& diagnosis) { diagnosis_ = diagnosis; }

    void set_host_port(const std::string& host, const std::string& port) {
        host_ = host;
        port_ = port;
    }
    const std::string& host() const { return host_; }
    const std::string& port() const { return port_; }

    bool client_request_failed() const { return !error_msg_.empty(); }
    ///
    /// @brief Replaces a diagnostic generated locally and clears any earlier server classification.
    ///
    /// @param[in] e Formatted diagnostic for the current failure.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    void set_error_msg(const std::string& e) {
        clear_error_codes();
        error_msg_ = e;
    }
    const std::string& error_msg() const { return error_msg_; }

    ///
    /// @brief Provides mutable diagnostic text for reply handlers that append server failures.
    ///
    /// @return Diagnostic text, valid until the next request or state reset.
    ///
    /// Unlike set_error_msg(), appending through this reference retains server error classification.
    ///
    std::string& get_error_msg() { return error_msg_; }

    ///
    /// @brief Reports whether a single server error reply supplied a numeric code.
    ///
    /// @return True only for a genuine single server ErrorCmd; zero is still present.
    /// Never throws.
    ///
    bool has_error_code() const noexcept { return std::holds_alternative<SingleError>(error_state_); }

    ///
    /// @brief Returns the raw code of the most recent single server error.
    ///
    /// @return Received code, including zero or an unknown future value; inspect has_error_code() first.
    /// Never throws.
    ///
    int error_code() const noexcept {
        const auto* error = std::get_if<SingleError>(&error_state_);
        return error ? error->code : 0;
    }

    ///
    /// @brief Returns the unformatted diagnostic from a single server ErrorCmd.
    ///
    /// @return Raw server text, valid until the next request or state reset.
    /// Never throws.
    ///
    const std::string& raw_error_message() const noexcept {
        const auto* error = std::get_if<SingleError>(&error_state_);
        static const std::string empty;
        return error ? error->message : empty;
    }

    ///
    /// @brief Records a genuine single server error reply.
    ///
    /// @param[in] code Raw numeric value, including zero or an unknown value.
    /// @param[in] message Unformatted server diagnostic text.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    void set_server_error(int code, const std::string& message) { error_state_.emplace<SingleError>(code, message); }

    ///
    /// @brief Marks the response as a group with individually coded errors.
    ///
    /// Never throws.
    ///
    void set_group_reply() noexcept { error_state_.emplace<GroupReply>(); }

    ///
    /// @brief Reports whether the response was a group reply.
    ///
    /// @return True for a GroupSTCCmd, including a group without failed children.
    /// Never throws.
    ///
    bool group_reply() const noexcept { return std::holds_alternative<GroupReply>(error_state_); }

    ///
    /// @brief Appends a failed child independently of a single reply code.
    ///
    /// @param[in] code Child's raw numeric code.
    /// @param[in] message Child's unformatted server diagnostic.
    /// @throws std::bad_alloc if appending the child fails.
    /// @throws std::bad_variant_access if set_group_reply() was not called first.
    ///
    void add_group_error(int code, const std::string& message) {
        std::get<GroupReply>(error_state_).entries.emplace_back(code, message);
    }

    ///
    /// @brief Returns ordered failed children for non-throwing callers.
    ///
    /// @return Failed child code and message entries, valid until the next request or state reset.
    /// Never throws.
    ///
    const std::vector<ecf::GroupError::Entry>& group_errors() const noexcept {
        const auto* group = std::get_if<GroupReply>(&error_state_);
        static const GroupErrors empty;
        return group ? group->entries : empty;
    }

    ///
    /// @brief Clears classified reply state after a local failure or before a request.
    ///
    /// Never throws.
    ///
    void clear_error_codes() noexcept { error_state_.emplace<std::monostate>(); }

    const std::vector<Zombie>& zombies() const { return zombies_; }
    void set_zombies(const std::vector<Zombie>& z) { zombies_ = z; }

    /// For uses with the suites cmd, or any other command that needs a vector of strings form server
    const std::vector<std::string>& get_string_vec() const { return str_vec_; }
    void set_string_vec(const std::vector<std::string>& z) { str_vec_ = z; }

    const std::vector<std::pair<unsigned int, std::vector<std::string>>>& get_client_handle_suites() const {
        return client_handle_suites_;
    }
    void set_client_handle_suites(const std::vector<std::pair<unsigned int, std::vector<std::string>>>& s) {
        client_handle_suites_ = s;
    }

    /// Only valid when the client requested the CFileCmd or log file from the server
    /// Other times this will be empty.
    const std::string& get_string() const { return str_; }
    void set_string(const std::string& f) { str_ = f; }

    /// Only valid when Stats command called.
    const Stats& stats() const { return stats_; }
    void set_stats(const Stats& s) { stats_ = s; }

    /// Return true if client is using Command Level Interface. Set via clear_for_invoke()
    bool cli() const { return cli_; }

    /// Only valid after a call to sync()
    /// in_sync() should be called after call to sync() Returns true if client defs is now in_sync with server
    bool in_sync() const { return in_sync_; }
    void set_sync(bool b) { in_sync_ = b; }

    /// The SSyncCmd will set if we have a full sync(ie retrieved full node tree from server) or incremental sync
    bool full_sync() const { return full_sync_; }
    void set_full_sync(bool f) { full_sync_ = f; }

    /// Only valid after a call to news()
    /// return true if the server has changed
    News_t get_news() const { return news_; }
    void set_news(News_t n) { news_ = n; }

    /// Only valid when the client requested the defs node tree from the server
    /// The defs *WILL* be retained until the next call to get the server defs
    /// **Hence is NOT reset in clear_for_invoke()
    defs_ptr client_defs() const { return client_defs_; }
    void set_client_defs(defs_ptr d) { client_defs_ = d; }

    /// Only valid when the client requested the node from the server
    /// The node *WILL* be retained until the next call to get the node
    /// **Hence is NOT reset in clear_for_invoke()
    node_ptr client_node() const { return client_node_; }
    void set_client_node(node_ptr d) { client_node_ = d; }

    /// A client handle of value 0 is not valid. Created in the server
    /// We should keep a reference to the client handle, for use with client handle commands
    /// **Hence is NOT reset in clear_for_invoke()
    void set_client_handle(int handle) { client_handle_ = handle; }
    int client_handle() const { return client_handle_; }

    // During incremental sync, record list of changed nodes, used by python api
    std::vector<std::string>& changed_nodes() { return changed_nodes_; }

private:
    friend class SSyncCmd;

    ///
    /// @brief Stores the raw code and diagnostic of one server ErrorCmd.
    ///
    /// Code zero is a present, unclassified server failure, not the absence of an error.
    ///
    struct SingleError
    {
        ///
        /// @brief Copies a single server failure into the reply state.
        ///
        /// @param[in] value Raw numeric code, including zero or an unknown future value.
        /// @param[in] text Unformatted server diagnostic.
        /// @throws std::bad_alloc if copying the diagnostic fails.
        ///
        SingleError(int value, const std::string& text)
            : code(value),
              message(text) {}

        /// Raw numeric classification of the server failure.
        int code;
        /// Unformatted server diagnostic of the failure.
        std::string message;
    };

    ///
    /// @brief Ordered raw code and diagnostic pairs for failed group children.
    ///
    using GroupErrors = std::vector<ecf::GroupError::Entry>;

    ///
    /// @brief Marks a group reply and stores its failed children, even when none failed.
    ///
    struct GroupReply
    {
        /// Failed child replies in their original reply order.
        GroupErrors entries;
    };

    std::string host_;      // clear at the start of invoke
    std::string port_;      // clear at the start of invoke
    std::string str_;       // clear at the start of invoke
    std::string error_msg_; // clear at the start of invoke
    std::variant<std::monostate, SingleError, GroupReply> error_state_;
    std::vector<Zombie> zombies_;            // clear at the start of invoke
    std::vector<std::string> str_vec_;       // clear at the start of invoke
    std::vector<std::string> changed_nodes_; // clear at the start of invoke
    std::vector<std::pair<unsigned int, std::vector<std::string>>>
        client_handle_suites_; // clear at the start of invoke
    Stats stats_;              // Used for test only, ideally need to clear

    defs_ptr client_defs_; // server reply stored as the client side defs, kept locally
    node_ptr client_node_; // server reply stored as the client side node, kept locally

    int client_handle_{0}; // set locally when suites are registered, and kept for reference
    News_t news_{NO_NEWS}; // clear at the start of invoke

    bool cli_{false};
    bool in_sync_{false};                      // clear at the start of invoke
    bool full_sync_{false};                    // clear at the start of invoke
    bool block_client_on_home_server_{false};  // clear at the start of invoke
    bool block_client_server_halted_{false};   // clear at the start of invoke
    bool block_client_zombie_detected_{false}; // clear at the start of invoke
    bool delete_all_{false};                   // clear at the start of invoke
    bool invalid_argument_{false};             // clear at the start of invoke
    bool eof_{false};                          // clear at the start of invoke

    ecf::ConnectionDiagnosis diagnosis_; // clear at the start of invoke
};
