// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include "ecflow/base/cts/user/UserCmd.hpp"

///
/// @brief Creates, reads, updates or deletes the Owner of a node (ecflow_client --ownership).
///
/// @details Reading is open to anyone with read access, and reports the node's own Owner, its effective Owner and
///          where it comes from, as key=value lines; on '/', the server Owner. Creating, updating and deleting are
///          write commands, decided by the ownership policy of the server (see ecf::OwnershipPolicy); the server
///          Owner is configuration, and cannot be changed by a command.
///
class OwnershipCmd final : public UserCmd {
public:
    ///
    /// @brief The operation requested.
    ///
    enum class Op { CREATE, READ, UPDATE, DELETE };

    ///
    /// @brief Constructs the command, checking its arguments.
    ///
    /// @param[in] op The operation.
    /// @param[in] path The absolute path of the node, or '/' (read only) for the server Owner.
    /// @param[in] value The new Owner, as user[:group]; required for create and update, and empty otherwise.
    /// @throws std::runtime_error if the path is not absolute, the value is missing, unexpected or malformed, or an
    ///         operation other than read is requested on '/'.
    ///
    OwnershipCmd(Op op, const std::string& path, const std::string& value = "");
    OwnershipCmd() = default;

    ///
    /// @brief Constructs the command without checking its arguments, as when received from a client.
    ///
    /// @param[in] op The operation.
    /// @param[in] path The path of the node.
    /// @param[in] value The new Owner, as given.
    /// @return The command; its arguments are checked again by the server.
    ///
    static OwnershipCmd make_unchecked(Op op, const std::string& path, const std::string& value);

    ///
    /// @brief Returns the name of an operation, as given on the command line.
    ///
    /// @param[in] op The operation.
    /// @return One of create, read, update, delete; "unknown" for a value outside the enumeration.
    ///
    static const char* to_string(Op op);

    ///
    /// @brief Returns the operation of a name given on the command line.
    ///
    /// @param[in] name The name of the operation.
    /// @return The operation, or no value when @p name is not one of create, read, update, delete.
    ///
    static std::optional<Op> to_op(const std::string& name);

    ///
    /// @brief Returns the first ecFlow version whose server supports this command.
    ///
    /// @return The version, as major.minor.patch.
    ///
    static const char* minimum_server_version();

    ///
    /// @brief Explains the error reported when this command is sent to a server that does not support it.
    ///
    /// @details A server without this command cannot decode the request, and replies with a generic error.
    ///
    /// @param[in] error The error reported for the request.
    /// @return An explicit message when @p error is that of a server unable to decode the request; no value
    ///         otherwise.
    ///
    static std::optional<std::string> explain_unsupported(const std::string& error);

    Op op() const { return op_; }
    const std::string& path() const { return path_; }
    const std::string& value() const { return value_; }

    bool isWrite() const override { return op_ != Op::READ; }

    using UserCmd::print;
    void print(std::string& os) const override;
    void print_only(std::string& os) const override;
    bool equals(ClientToServerCmd* rhs) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override { return arg(); }

    ///
    /// @brief Registers the --ownership option (multi-token: operation, path and optional value).
    ///
    /// @param[in,out] desc The description the option is added to.
    ///
    void addOption(boost::program_options::options_description& desc) const override;

    ///
    /// @brief Creates the command from the parsed command line options.
    ///
    /// @param[out] cmd The created command.
    /// @param[in] vm The parsed options.
    /// @param[in] clientEnv The client environment.
    /// @throws std::runtime_error if the operation is unknown, the number of arguments is wrong for the operation,
    ///         or the arguments are invalid.
    ///
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg(); // used for argument parsing

    STC_Cmd_ptr doHandleRequest(AbstractServer* as) const override;
    STC_Cmd_ptr read(AbstractServer* as) const;

    Op op_{Op::READ};
    std::string path_;
    std::string value_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(op_), CEREAL_NVP(path_), CEREAL_NVP(value_));
    }
};

std::ostream& operator<<(std::ostream& os, const OwnershipCmd&);

CEREAL_FORCE_DYNAMIC_INIT(OwnershipCmd)
