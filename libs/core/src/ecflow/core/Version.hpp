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
/// @brief Provides information regarding the version of ecFlow.
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

    ///
    /// @brief Return the major version component of ecFlow.
    ///
    /// @return The major version component as a string.
    ///
    static std::string major();

    ///
    /// @brief Return the minor version component of ecFlow.
    ///
    /// @return The minor version component as a string.
    ///
    static std::string minor();

    ///
    /// @brief Return the patch version component of ecFlow.
    ///
    /// @return The patch version component as a string.
    ///
    static std::string patch();

    ///
    /// @brief Return the version suffix of ecFlow, if any.
    ///
    /// @return The version suffix as a string, or an empty string if no suffix is defined.
    ///
    static std::string suffix();

    ///
    /// @brief Return a descriptive version string, including ecFlow and relevant dependency versions.
    ///
    /// This provides user-facing version information
    /// (shown by ecflow_client --help, and in the server info panel on ecflow_ui).
    ///
    /// @return A formatted version description string.
    ///
    static std::string description();

    ///
    /// @brief Return the ecFlow version string, without any suffix.
    ///
    /// Format: `<major>.<minor>.<patch>`
    ///
    /// @return The base version string.
    ///
    static std::string base();

    ///
    /// @brief Return the complete ecFlow version string, including any suffix.
    ///
    /// Format: `<major>.<minor>.<patch>[<suffix>]`
    ///
    /// @return The full version string.
    ///
    static std::string full();

private:
    ///
    /// @brief Return the version of the Boost library.
    ///
    /// @return The Boost library version as a string.
    ///
    static std::string boost();

    ///
    /// @brief Return the version of the Cereal library.
    ///
    /// @return The Cereal library version as a string.
    ///
    static std::string cereal();

    ///
    /// @brief Return the version of the compiler used to build ecFlow.
    ///
    /// @return The compiler version as a string.
    ///
    static std::string compiler();

    ///
    /// @brief Return the version of the OpenSSL library.
    ///
    /// @return The OpenSSL library version as a string.
    ///
    static std::string openssl();
};

} // namespace ecf

#endif /* ecflow_core_Version_HPP */
