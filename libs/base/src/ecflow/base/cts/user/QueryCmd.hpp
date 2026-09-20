// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_base_cts_user_QueryCmd_HPP
#define ecflow_base_cts_user_QueryCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"
#include "ecflow/core/cereal_optional_nvp.hpp"

class QueryCmd final : public UserCmd {
public:
    ///
    /// @brief Constructs a query command.
    ///
    /// @param[in] query_type the kind of query, one of [state | dstate | repeat | event | meter | limit |
    ///                       limit_max | label | variable | trigger]
    /// @param[in] path_to_attribute the path to the node holding the queried attribute ('/' for the server)
    /// @param[in] attribute the attribute name, or the trigger expression; empty for state and dstate
    /// @param[in] path_to_task the task invoking the command (used for logging only, may be empty)
    /// @param[in] evaluate when true, and only for query type 'variable', the variable value is returned with
    ///                     all variable references (e.g. %VAR%) resolved, instead of as stored
    ///
    QueryCmd(const std::string& query_type,
             const std::string& path_to_attribute,
             const std::string& attribute,
             const std::string& path_to_task,
             bool evaluate = false)
        : query_type_(query_type),
          path_to_attribute_(path_to_attribute),
          attribute_(attribute),
          path_to_task_(path_to_task),
          evaluate_(evaluate) {}
    QueryCmd()
        : UserCmd() {}
    ~QueryCmd() override;

    const std::string& query_type() const { return query_type_; }
    const std::string& path_to_attribute() const { return path_to_attribute_; }
    const std::string& attribute() const { return attribute_; }
    const std::string& path_to_task() const { return path_to_task_; }

    ///
    /// @brief Indicates whether the variable references in the queried value are to be resolved.
    ///
    /// @return true when --evaluate was requested; only meaningful for query type 'variable'
    ///
    bool evaluate() const { return evaluate_; }

    ///
    /// @brief Appends the command, as logged by the server, followed by the user and client host.
    ///
    /// @param[out] os the string the command is appended to
    ///
    void print(std::string& os) const override;

    ///
    /// @brief Appends the command as logged by the server, without user and client host.
    ///
    /// @param[out] os the string the command is appended to
    ///
    void print_only(std::string& os) const override;

    ///
    /// @brief Compares this command with another one.
    ///
    /// @param[in] rhs the command to compare with
    /// @return true when @p rhs is a QueryCmd with the same query type, path, attribute, calling task path and
    ///         evaluate flag, and the base class considers the commands equal
    ///
    bool equals(ClientToServerCmd* rhs) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override { return arg(); }

    ///
    /// @brief Registers the --query option (multi-token) and its --evaluate modifier (value-less).
    ///
    /// @param[in,out] desc the description the options are added to
    ///
    void addOption(boost::program_options::options_description& desc) const override;

    ///
    /// @brief Creates the command from the parsed command line options.
    ///
    /// @param[out] cmd the created command
    /// @param[in] vm the parsed options; the --query tokens are validated per query type, and --evaluate is
    ///               accepted only with query type 'variable'
    /// @param[in] clientEnv the client environment, providing the calling task path (ECF_NAME) for logging
    /// @throws std::runtime_error when the query type, its arguments or the --evaluate combination are invalid
    ///
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg(); // used for argument parsing

    ///
    /// @brief Renders the command as logged: the query, followed by the calling task path when present.
    ///
    /// @return the command text, without user and host
    ///
    std::string print_as_string() const;

    bool handleRequestIsTestable() const override { return false; }

    ///
    /// @brief Dispatches the query to the handler of its query type.
    ///
    /// @param[in] as the server holding the definition
    /// @return the reply carrying the queried value as a string
    /// @throws std::runtime_error when --evaluate is combined with a query type other than 'variable', or when
    ///         the query type is unknown
    ///
    STC_Cmd_ptr doHandleRequest(AbstractServer* as) const override;

    // Handling of each query type is delegated to one of the functions below, keeping
    // doHandleRequest() as a simple dispatch on query_type_.
    //
    // Note: 'state' and 'variable' are the only query types that can be addressed at the server
    // itself (i.e. when path_to_attribute_ is '/'), so their handlers deal with that case directly.
    // All other handlers only ever operate on a node, found via find_node_for_query().
    node_ptr find_node_for_query(Defs*) const;
    STC_Cmd_ptr doHandleQueryForState(Defs*) const;
    STC_Cmd_ptr doHandleQueryForDState(Defs*) const;
    STC_Cmd_ptr doHandleQueryForRepeat(Defs*) const;
    STC_Cmd_ptr doHandleQueryForEvent(Defs*) const;
    STC_Cmd_ptr doHandleQueryForMeter(Defs*) const;
    STC_Cmd_ptr doHandleQueryForLimit(Defs*) const;
    STC_Cmd_ptr doHandleQueryForLimitMax(Defs*) const;
    STC_Cmd_ptr doHandleQueryForLabel(Defs*) const;

    ///
    /// @brief Returns the value of a user, repeat or generated variable, searching up the node tree, or of a
    ///        server variable when the path is '/'.
    ///
    /// @param[in] defs the definition being queried
    /// @return the reply carrying the value as stored or, when evaluate() is true, with all variable references
    ///         resolved
    /// @throws std::runtime_error when the variable is not found, or when evaluate() is true and a reference in
    ///         the value cannot be resolved (a partially resolved value is never returned)
    ///
    STC_Cmd_ptr doHandleQueryForVariable(Defs* defs) const;
    STC_Cmd_ptr doHandleQueryForTrigger(Defs*) const;

private:
    std::string query_type_; // [ state | dstate | event | meter | label | trigger ]
    std::string path_to_attribute_;
    std::string attribute_;    // [ event_name | meter_name | label_name | variable_name | trigger expression] empty for
                               // state and dstate
    std::string path_to_task_; // The task the invoked this command, needed for logging
    bool evaluate_{false};     // Only meaningful for query type 'variable': resolve variable references in the value

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(query_type_),
           CEREAL_NVP(path_to_attribute_),
           CEREAL_NVP(attribute_),
           CEREAL_NVP(path_to_task_));
        CEREAL_OPTIONAL_NVP(ar, evaluate_, [this]() { return evaluate_; }); // conditionally save
    }
};

std::ostream& operator<<(std::ostream& os, const QueryCmd&);

CEREAL_FORCE_DYNAMIC_INIT(QueryCmd)

#endif /* ecflow_base_cts_user_QueryCmd_HPP */
