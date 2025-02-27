/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Version_HPP
#define ecflow_core_Version_HPP

#include <string>

namespace ecf {

///
/// \brief Provides information regarding the version of ecFlow.
///
/// The ecFlow version format is of the form `<major>.<minor>.<patch>[<suffix>]`,
/// following the semantic versioning scheme.
///
/// The version is defined in the CMakeLists.txt file.
///
class Version {
public:
    // Disable default construction
    Version() = delete;
    // Disable copy (and move) semantics
    Version(const Version&)                  = delete;
    const Version& operator=(const Version&) = delete;

    ///
    /// Creates a string with a descriptive version information,
    /// including the version of ecFlow and relevant dependencies.
    ///
    /// This provides user facing version information
    /// (shown by ecflow_client --help, and in the server info panel on ecflow_ui).
    ///
    static std::string description();

    ///
    /// Creates the ecFlow version, following the template: `ecflow_<release>_<major>_<minor>`
    ///
    static std::string version();

    ///
    /// Creates the ecFlow version, following the template: `<release>.<major>.<minor>`
    ///
    static std::string raw();

    ///
    /// Creates the ecFlow version, following the template: `<release>.<major>.<minor>[<suffix>]`
    ///
    static std::string full();

    static std::string major();
    static std::string minor();
    static std::string patch();
    static std::string suffix();

private:
    /// Create a string containing the version of the Boost library
    static std::string boost();

    /// Create a string containing the version of the Cereal library
    static std::string cereal();

    /// Create a string containing the version of the Compiler used to build ecFlow
    static std::string compiler();
};

} // namespace ecf

#endif /* ecflow_core_Version_HPP */
