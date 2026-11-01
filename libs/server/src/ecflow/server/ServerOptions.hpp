// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <string>

#include <boost/program_options.hpp>

#include "ecflow/core/CommandLine.hpp"

///
/// @brief Parses the syntax of the server command line; the values are checked by the server catalogue.
///
class ServerOptions {
public:
    ///
    /// @brief Parses command-line options and prints requested help or version information.
    ///
    /// No server configuration, environment settings or runtime files are accessed.
    ///
    /// @param[in] cl Command-line tokens including the executable name
    /// @throws ServerEnvironmentException if command-line syntax or numeric conversion fails, preserving the parser
    /// diagnostic.
    ///
    explicit ServerOptions(const CommandLine& cl);

    ///
    /// @brief Returns the options given, as read by ecf::cfg::CliReader.
    ///
    /// @return The options given, by long name, each with its text; the text of a switch is empty.
    ///
    [[nodiscard]] std::map<std::string, std::string> options() const;

    // Disable copy (and move) semantics
    ServerOptions(const ServerOptions&)                  = delete;
    const ServerOptions& operator=(const ServerOptions&) = delete;
    ServerOptions(ServerOptions&&)                       = delete;
    ServerOptions& operator=(ServerOptions&&)            = delete;

    ~ServerOptions() = default;

    ///
    /// @brief Reports whether help output was requested.
    ///
    /// @return True if either help spelling was supplied.
    ///
    bool help_option() const;

    ///
    /// @brief Reports whether version output was requested.
    ///
    /// @return True if either version spelling was supplied.
    ///
    bool version_option() const;

private:
    boost::program_options::variables_map vm_;
};
