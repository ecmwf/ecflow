/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_HelpCatalog_HPP
#define ecflow_client_HelpCatalog_HPP

#include <string>

#include <nlohmann/json.hpp>

namespace ecf {

///
/// @brief Provides lazy, cached access to the embedded CLI help manifest.
///
/// Allows access to the manifest (docs/client_api/help.json).
/// This is embedded at build time into `generated_client_help.hpp` and is parsed
/// only on first use and cached for the remaining lifetime of the process.
///
/// The lazy nature of the load matters because ecflow_client is invoked by a task,
/// potentially thousands of times over a suite run; only a small fraction of those
/// invocations ever request help, and the rest must not pay a JSON-parsing cost.
///
class HelpCatalog {
public:
    HelpCatalog()                              = delete;
    HelpCatalog(const HelpCatalog&)            = delete;
    HelpCatalog& operator=(const HelpCatalog&) = delete;

    ///
    /// @brief Returns the parsed manifest, parsing it once on the first call.
    ///
    /// @return Reference to the manifest, valid for the remaining lifetime of the process.
    /// @throws nlohmann::json::parse_error if the embedded manifest is malformed.
    ///
    static const nlohmann::json& manifest();

    ///
    /// @brief Finds a command entry by exact name.
    ///
    /// @param name Exact command name to look up, e.g. "abort".
    /// @return Pointer to the matching manifest entry, valid for the remaining lifetime
    ///         of the process, or nullptr if no command has this name.
    ///
    static const nlohmann::json* find_command(const std::string& name);

    ///
    /// @brief Finds an option entry by exact name.
    ///
    /// @param name Exact option name to look up, e.g. "host".
    /// @return Pointer to the matching manifest entry, valid for the remaining lifetime
    ///         of the process, or nullptr if no option has this name.
    ///
    static const nlohmann::json* find_option(const std::string& name);

    ///
    /// @brief Finds a topic entry by exact name.
    ///
    /// @param name Exact topic name to look up, e.g. "summary".
    /// @return Pointer to the matching manifest entry, valid for the remaining lifetime
    ///         of the process, or nullptr if no topic has this name.
    ///
    static const nlohmann::json* find_topic(const std::string& name);

private:
    static const nlohmann::json* find_by_name(const nlohmann::json& array, const std::string& name);
};

} // namespace ecf

#endif /* ecflow_client_HelpCatalog_HPP */
