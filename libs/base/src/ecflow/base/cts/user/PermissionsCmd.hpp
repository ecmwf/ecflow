// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/cts/user/UserCmd.hpp"

///
/// @brief Reads and changes the Permissions attribute of a node, or of the server (--permissions).
///
/// The operations act on the whole list of the attribute, at exactly one path, "/" addressing the server attribute
/// (D4.3, D4.4, D5.23, D5.24):
///
/// - create: gives the node an attribute; fails when the node already has one;
/// - read: returns the attribute; fails when the node has none;
/// - update: replaces the attribute; fails when the node has none;
/// - delete: removes the attribute; fails when the node has none;
/// - effective: returns the effective roles of a user at the path, the requester by default.
///
/// At the server, create and delete fail: the server attribute is configured at start-up only (D10.2). Changes go
/// through the validation of changes (ecf::PermissionsChange).
///
class PermissionsCmd final : public UserCmd {
public:
    ///
    /// @brief Identifies the format of the values returned by read and effective.
    ///
    enum class Format {
        Cli, ///< The command-line form, e.g. "alice:rwx,bob:r", or the letters of the roles for effective
        Json ///< The JSON form
    };

    ///
    /// @brief Creates the command.
    ///
    /// @param[in] operation One of create, read, update, delete or effective
    /// @param[in] path The path of a node, or "/" for the server
    /// @param[in] value For create and update, the attribute, in the command-line or the JSON form; for effective,
    ///            the user reported, the requester when empty; empty otherwise
    /// @param[in] format The format of the values returned
    ///
    PermissionsCmd(std::string operation, std::string path, std::string value = {}, Format format = Format::Cli);
    PermissionsCmd() = default;

    ///
    /// @brief Returns the operation: create, read, update, delete or effective.
    ///
    /// @return The operation, as given to the constructor.
    ///
    [[nodiscard]] const std::string& operation() const { return operation_; }

    ///
    /// @brief Returns the path of the node, or "/" for the server.
    ///
    /// @return The path, as given to the constructor.
    ///
    [[nodiscard]] const std::string& path() const { return path_; }

    ///
    /// @brief Returns the attribute given to create and update, or the user given to effective.
    ///
    /// @return The value; empty when none was given.
    ///
    [[nodiscard]] const std::string& value() const { return value_; }

    ///
    /// @brief Returns the format of the values returned by read and effective.
    ///
    /// @return The format.
    ///
    [[nodiscard]] Format format() const { return format_; }

    ///
    /// @brief Returns whether the operation changes an attribute: create, update or delete.
    ///
    /// @return True for the operations that require p.
    ///
    [[nodiscard]] bool changes() const;

    ///
    /// @brief Checks the arguments of the command, on the client and again on the server.
    ///
    /// @param[in] operation One of create, read, update, delete or effective
    /// @param[in] path An absolute path: "/" or the path of a node
    /// @param[in] value Required by create and update, and then a valid attribute; refused by read and delete;
    ///            optional for effective, where it names a user
    /// @throws std::runtime_error naming the argument at fault.
    ///
    static void check_arguments(const std::string& operation, const std::string& path, const std::string& value);

    ///
    /// @brief Returns whether the command changes the definitions: true for create, update and delete.
    ///
    /// @return The same as changes(); a change is checkpointed, recorded in the edit history, and followed by the
    ///         automatic synchronisation of the client, when enabled.
    ///
    bool isWrite() const override { return changes(); }

    using UserCmd::print;

    /// @copydoc ClientToServerCmd::print
    void print(std::string& os) const override;

    /// @copydoc ClientToServerCmd::print_only
    void print_only(std::string& os) const override;

    ///
    /// @brief Compares the command with another one.
    ///
    /// @param[in] rhs The other command
    /// @return True when rhs is a PermissionsCmd with the same operation, path, value and format.
    ///
    bool equals(ClientToServerCmd* rhs) const override;

    /// @copydoc ClientToServerCmd::authenticate
    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;

    ///
    /// @brief Authorises the command: r at the path for read and effective, p for create, update and delete.
    ///
    /// @param[in] context The context of the request
    /// @return Success when allowed; otherwise a failure holding the reason.
    ///
    [[nodiscard]] ecf::authorisation_t authorise(const ecf::RequestContext& context) const override;

    /// @copydoc ClientToServerCmd::theArg
    const char* theArg() const override { return arg(); }

    ///
    /// @brief Registers the option --permissions (several values) and its modifier --fmt.
    ///
    /// @param[in,out] desc The description the options are added to
    ///
    void addOption(boost::program_options::options_description& desc) const override;

    ///
    /// @brief Creates the command from the command line: --permissions <operation> <path> [<value>] [--fmt cli|json].
    ///
    /// @param[out] cmd The command created
    /// @param[in] vm The parsed options
    /// @param[in] clientEnv The client environment
    /// @throws std::runtime_error when the arguments are invalid (see check_arguments()), or the format unknown.
    ///
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

    ///
    /// @brief Parses the value of create and update, in the JSON form or in the command-line form.
    ///
    /// The JSON form is tried first; a value that is not valid JSON is parsed as the command-line form. The two forms
    /// cannot be confused: the JSON form holds characters that user names cannot hold (":" and ",").
    ///
    /// @param[in] value The attribute
    /// @return The attribute.
    /// @throws std::runtime_error when the value is valid in neither form, with the error of the command-line form,
    ///         and that of the JSON form when the value starts with "[".
    ///
    static ecf::Permissions parse_value(const std::string& value);

private:
    static const char* arg();
    [[nodiscard]] std::string print_as_string() const;
    bool handleRequestIsTestable() const override { return false; }
    STC_Cmd_ptr doHandleRequest(AbstractServer* as) const override;

    std::string operation_;
    std::string path_;
    std::string value_;
    Format format_{Format::Cli};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(operation_),
           CEREAL_NVP(path_),
           CEREAL_NVP(value_),
           CEREAL_NVP(format_));
    }
};

std::ostream& operator<<(std::ostream& os, const PermissionsCmd&);

CEREAL_FORCE_DYNAMIC_INIT(PermissionsCmd)
