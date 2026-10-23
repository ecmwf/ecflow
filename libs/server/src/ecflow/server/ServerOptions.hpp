// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <boost/program_options.hpp>

#include "ecflow/core/CommandLine.hpp"

class ServerEnvironment;

///
/// @brief Parses server command-line syntax separately from applying runtime overrides.
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
    /// @brief Applies parsed overrides after configuration and environment settings have been selected.
    ///
    /// @param[in,out] env Non-null server environment receiving command-line and transport settings
    /// @throws std::exception if SSL selection or setup fails.
    ///
    void apply(ServerEnvironment* env) const;

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
