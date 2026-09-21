// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "ecflow/base/Cmd.hpp"

class AbstractClientEnv;

///
/// \brief Registration of all the client to server commands
///
/// This allows us to parse the arg associated with commands in this category.
/// The idea is to keep new commands functionality in one place.
/// Any new client to server commands that are created must be added to this class.
///

class CtsCmdRegistry {
public:
    ///
    /// @brief Registers every client to server command, and collects their modifier options.
    ///
    /// @param[in] addGroupCmd whether the --group command is registered (it is not, when the registry serves the
    ///                        parsing of the sub-commands of a group)
    ///
    explicit CtsCmdRegistry(bool addGroupCmd = true);

    CtsCmdRegistry(const CtsCmdRegistry&)            = delete;
    CtsCmdRegistry& operator=(const CtsCmdRegistry&) = delete;
    CtsCmdRegistry(CtsCmdRegistry&&)                 = delete;
    CtsCmdRegistry& operator=(CtsCmdRegistry&&)      = delete;

    ~CtsCmdRegistry() = default;

    /// These option describe the arguments for each of the commands
    /// They also can be presented to the user via --help option.
    void addAllOptions(boost::program_options::options_description& desc) const;
    void addCmdOptions(boost::program_options::options_description& desc) const;

    ///
    /// @brief Creates the command selected by the parsed options.
    ///
    /// The first registered command whose argument is present in @p vm is created. A modifier option owned by
    /// another command (see modifiers_) is rejected beforehand, so that it is never silently discarded.
    ///
    /// @param[out] cmd the created command; left unset when the matched command is client specific and nothing
    ///                 is to be sent to the server
    /// @param[in] vm the parsed command line options
    /// @param[in] clientEnv the client environment
    /// @return true when an option in @p vm matches a registered command (even if @p cmd is left unset)
    /// @throws std::runtime_error when a modifier is given with a command that does not own it, or when the
    ///         matched command rejects its arguments
    ///
    bool parse(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const;

private:
    std::vector<Cmd_ptr> vec_;

    /// Modifier options, i.e. options registered by a command in addition to its own argument (for example
    /// --evaluate, registered by --query), mapped to the argument of the command that owns them. A modifier
    /// is only meaningful together with its owning command, and is rejected when given with any other command.
    std::map<std::string, std::string> modifiers_;

    void addHelpOption(boost::program_options::options_description& desc) const;

    ///
    /// @brief Collects the modifier options registered by every command into modifiers_.
    ///
    void collectModifiers();

    ///
    /// @brief Rejects any modifier option present in `vm` that is not owned by the command being created.
    ///
    /// @param[in] matched_arg the argument of the command selected for creation
    /// @param[in] vm the parsed command line options
    /// @throws std::runtime_error when a modifier owned by another command is present
    ///
    void rejectForeignModifiers(const std::string& matched_arg, const boost::program_options::variables_map& vm) const;
};
