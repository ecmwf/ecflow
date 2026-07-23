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
};

} // namespace ecf

#endif /* ecflow_client_HelpCatalog_HPP */
