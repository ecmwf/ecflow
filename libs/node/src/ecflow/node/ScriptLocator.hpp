// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include "ecflow/node/EcfFile.hpp"

namespace ecf {

class VariableLookup;

///
/// @brief Where the script of a task or an alias was found, as the server's search finds it.
///
struct ScriptLocation
{
    /// Where the script comes from
    EcfFile::Origin origin{EcfFile::ECF_SCRIPT};
    /// The path of the script, or, for ECF_FETCH_CMD and ECF_SCRIPT_CMD, the command that produces it, substituted
    std::string path_or_cmd;
    /// The search used in ECF_FILES or ECF_HOME
    EcfFile::EcfFileSearchAlgorithm algorithm{EcfFile::PRUNE_ROOT};

    ///
    /// @brief Tells whether the script is obtained by running a command, ECF_FETCH or ECF_SCRIPT_CMD.
    ///
    /// @return True for ECF_FETCH_CMD and ECF_SCRIPT_CMD.
    ///
    [[nodiscard]] bool needs_command() const {
        return origin == EcfFile::ECF_FETCH_CMD || origin == EcfFile::ECF_SCRIPT_CMD;
    }
};

///
/// @brief Locates the script of a task or an alias, as the server does, from its variables only.
///
/// The places are tried in the server's order: the generated ECF_SCRIPT (ECF_HOME, the node path and the extension);
/// ECF_FETCH; ECF_SCRIPT_CMD; a search of ECF_FILES (after variable substitution when the directory does not exist as
/// written); a search of ECF_HOME. ECF_FILES_LOOKUP selects pruning the root (the default) or the leaf. The search
/// never runs ECF_FETCH or ECF_SCRIPT_CMD: it only reports them, substituted.
///
/// As in the server, ECF_FETCH and ECF_SCRIPT_CMD are read with VariableLookup::find, so a generated variable of the
/// same name, such as a repeat, counts; ECF_HOME, ECF_FILES, ECF_FILES_LOOKUP and ECF_MICRO are read with
/// VariableLookup::find_user, so only user variables count. ECF_SCRIPT is computed from ECF_HOME, the node path and
/// the extension, not read. The substitutions use every variable of @p lookup, generated ones included (ECF_NAME,
/// TASK, ...), so the generated variables of the lookup must be current; for a node, update them before building the
/// lookup, as Submittable::locatedEcfFile does.
///
/// @param[in] abs_node_path The absolute path of the node, such as /suite/family/task.
/// @param[in] extension The extension of the script, `.ecf` for a task and `.usr` for an alias.
/// @param[in] lookup The variables as seen from the node.
/// @return The location of the script.
/// @throws std::runtime_error when no script is found, or when ECF_FETCH or ECF_SCRIPT_CMD cannot be substituted;
///         the message lists every place that was tried, as the server reports it.
///
ScriptLocation
locate_script(const std::string& abs_node_path, const std::string& extension, const VariableLookup& lookup);

///
/// @brief Searches ECF_FILES for the manual (`.man` file) of a family or a suite, as the server does first.
///
/// The server reads the manual found here. When none is found, or when it gives an empty manual, and only then, the
/// server searches ECF_HOME, as locate_manual_in_ecf_home does; a caller follows the same order, so that ECF_HOME is
/// not examined when this manual is used. ECF_FILES is read with VariableLookup::find_user and used without variable
/// substitution; the search prunes the root of the node path (ECF_FILES_LOOKUP is not used).
///
/// @param[in] abs_node_path The absolute path of the family or the suite, such as /suite/family.
/// @param[in] lookup The variables as seen from the node.
/// @return The path of the manual; empty when ECF_FILES is not defined, is not a directory or holds no manual.
/// @throws std::filesystem::filesystem_error when ECF_FILES cannot be examined, as in the server.
///
std::string locate_manual_in_ecf_files(const std::string& abs_node_path, const VariableLookup& lookup);

///
/// @brief Searches ECF_HOME for the manual (`.man` file) of a family or a suite, as the server does second.
///
/// See locate_manual_in_ecf_files for the order. ECF_HOME is read with VariableLookup::find_user and used without
/// variable substitution; the search prunes the root of the node path.
///
/// @param[in] abs_node_path The absolute path of the family or the suite, such as /suite/family.
/// @param[in] lookup The variables as seen from the node.
/// @return The path of the manual; empty when ECF_HOME is a directory that holds no manual, in which case the server
///         tries to read an empty path and fails; no value when ECF_HOME is not defined or is not a directory, in
///         which case the server reports that there is no manual.
/// @throws std::filesystem::filesystem_error when ECF_HOME cannot be examined, as in the server.
///
std::optional<std::string> locate_manual_in_ecf_home(const std::string& abs_node_path, const VariableLookup& lookup);

} // namespace ecf
