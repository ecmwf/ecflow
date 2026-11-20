// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/ScriptLocator.hpp"

#include <sstream>
#include <stdexcept>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/node/VariableLookup.hpp"

namespace ecf {

namespace {

/// Substitutes a command as Node::variableSubstitution does: no user edit variables, and ECF_MICRO as the micro
bool substitute(std::string& cmd, const VariableLookup& lookup) {
    char micro = '%';
    std::string micro_char;
    lookup.find_user(ecf::environment::ECF_MICRO, micro_char);
    if (!micro_char.empty() && micro_char.size() == 1) {
        micro = micro_char.front();
    }
    NameValueMap user_edit_variables;
    return variable_substitution(cmd, lookup, user_edit_variables, micro);
}

std::string search(const std::string& directory,
                   const std::string& abs_node_path,
                   const std::string& extension,
                   EcfFile::EcfFileSearchAlgorithm algorithm) {
    if (algorithm == EcfFile::PRUNE_ROOT) {
        return File::backwardSearch(directory, abs_node_path, extension);
    }
    return File::forwardSearch(directory, abs_node_path, extension);
}

} // namespace

ScriptLocation
locate_script(const std::string& abs_node_path, const std::string& extension, const VariableLookup& lookup) {
    std::string reasonEcfFileNotFound;
    std::string ecf_home;
    lookup.find_user(ecf::environment::ECF_HOME, ecf_home);

    // The generated ECF_SCRIPT: ECF_HOME, the node path and the extension
    const std::string ecf_script = ecf_home + abs_node_path + extension;
    if (fs::exists(ecf_script)) {
        return {EcfFile::ECF_SCRIPT, ecf_script, EcfFile::PRUNE_ROOT};
    }
    reasonEcfFileNotFound += "   ECF_SCRIPT(";
    reasonEcfFileNotFound += ecf_script;
    reasonEcfFileNotFound += ") does not exist:\n";

    // ECF_FETCH is not used in operations or research
    std::string ecf_fetch_cmd;
    lookup.find(ecf::environment::ECF_FETCH, ecf_fetch_cmd);
    if (!ecf_fetch_cmd.empty()) {
        if (substitute(ecf_fetch_cmd, lookup)) {
            return {EcfFile::ECF_FETCH_CMD, ecf_fetch_cmd, EcfFile::PRUNE_ROOT};
        }
        reasonEcfFileNotFound += "   Variable ECF_FETCH(";
        reasonEcfFileNotFound += ecf_fetch_cmd;
        reasonEcfFileNotFound += ") defined, but variable substitution has failed:\n";
        throw std::runtime_error(reasonEcfFileNotFound);
    }
    reasonEcfFileNotFound += "   Variable ECF_FETCH not defined:\n";

    std::string ecf_script_cmd; // ECFLOW-427
    lookup.find("ECF_SCRIPT_CMD", ecf_script_cmd);
    if (!ecf_script_cmd.empty()) {
        if (substitute(ecf_script_cmd, lookup)) {
            return {EcfFile::ECF_SCRIPT_CMD, ecf_script_cmd, EcfFile::PRUNE_ROOT};
        }
        reasonEcfFileNotFound += "   Variable ECF_SCRIPT_CMD(";
        reasonEcfFileNotFound += ecf_script_cmd;
        reasonEcfFileNotFound += ") defined, but variable substitution has failed:\n";
        throw std::runtime_error(reasonEcfFileNotFound);
    }
    reasonEcfFileNotFound += "   Variable ECF_SCRIPT_CMD not defined:\n";

    EcfFile::EcfFileSearchAlgorithm file_search_algo = EcfFile::PRUNE_ROOT;
    std::string ecf_files_lookup;
    if (lookup.find_user("ECF_FILES_LOOKUP", ecf_files_lookup)) {
        if (ecf_files_lookup == "prune_leaf" || ecf_files_lookup == "PRUNE_LEAF") {
            file_search_algo = EcfFile::PRUNE_LEAF;
        }
    }

    std::string ecf_filesDirectory;
    if (lookup.find_user(ecf::environment::ECF_FILES, ecf_filesDirectory)) {
        if (!ecf_filesDirectory.empty() && fs::is_directory(ecf_filesDirectory)) {
            // An empty result is a failure to locate the script
            std::string searchResult = search(ecf_filesDirectory, abs_node_path, extension, file_search_algo);
            if (searchResult.empty()) {
                reasonEcfFileNotFound += "   Search of directory ECF_FILES(";
                reasonEcfFileNotFound += ecf_filesDirectory;
                reasonEcfFileNotFound += ") failed ";
                if (file_search_algo == EcfFile::PRUNE_ROOT) {
                    reasonEcfFileNotFound += "using prune_root:\n";
                }
                else {
                    reasonEcfFileNotFound += "using prune_leaf:\n";
                }
            }
            else {
                return {EcfFile::ECF_FILES, searchResult, file_search_algo};
            }
        }
        else {
            // Before failing try again but with variable Substitution. ECFLOW-788
            std::string original_ecf_filesDirectory = ecf_filesDirectory;
            substitute(ecf_filesDirectory, lookup);
            if (!ecf_filesDirectory.empty() && fs::is_directory(ecf_filesDirectory)) {
                std::string searchResult = search(ecf_filesDirectory, abs_node_path, extension, file_search_algo);
                if (searchResult.empty()) {
                    reasonEcfFileNotFound += MESSAGE("   Search of directory ECF_FILES(variable substituted)("
                                                     << ecf_filesDirectory << ") failed:\n");
                }
                else {
                    return {EcfFile::ECF_FILES, searchResult, file_search_algo};
                }
            }
            else {
                std::ostringstream ss;
                ss << "   Directory ECF_FILES(" << original_ecf_filesDirectory << ") does not exist:\n";
                if (original_ecf_filesDirectory != ecf_filesDirectory) {
                    ss << "   Directory ECF_FILES(" << ecf_filesDirectory
                       << ") after variable substitution does not exist:\n";
                }
                reasonEcfFileNotFound += ss.str();
            }
        }
    }
    else {
        reasonEcfFileNotFound += "   Variable ECF_FILES not defined:\n";
    }

    if (!ecf_home.empty() && fs::is_directory(ecf_home)) {
        std::string searchResult = search(ecf_home, abs_node_path, extension, file_search_algo);
        if (searchResult.empty()) {
            reasonEcfFileNotFound += "   Search of directory ECF_HOME(";
            reasonEcfFileNotFound += ecf_home;
            reasonEcfFileNotFound += ") failed ";
            if (file_search_algo == EcfFile::PRUNE_ROOT) {
                reasonEcfFileNotFound += "using prune_root:\n";
            }
            else {
                reasonEcfFileNotFound += "using prune_leaf:\n";
            }
        }
        else {
            return {EcfFile::ECF_HOME, searchResult, file_search_algo};
        }
    }
    else {
        reasonEcfFileNotFound += "   Directory ECF_HOME(";
        reasonEcfFileNotFound += ecf_home;
        reasonEcfFileNotFound += ") does not exist:\n";
    }

    // failed to find  .ecf file
    std::string error_msg = "   Script for ";
    error_msg += abs_node_path;
    error_msg += " cannot be found:\n";
    error_msg += reasonEcfFileNotFound;
    throw std::runtime_error(error_msg);
}

} // namespace ecf
