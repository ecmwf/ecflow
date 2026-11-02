// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>

#include "ecflow/core/cfg/Catalogue.hpp"
#include "ecflow/core/cfg/Configuration.hpp"

namespace ecf::cfg {

///
/// @brief Reads `server_environment.cfg` into a layer of settings of a catalogue.
///
/// The file holds lines of the form `NAME = value`, with comments following `#`. Each name is the name of a setting
/// in `server_environment.cfg` (Names::env_cfg), and each value is turned into a value of its setting by
/// Setting::parse().
///
/// The reader refuses the following, naming the file:
///
///   - a path that cannot be inspected, is not a regular file, or cannot be read;
///   - a malformed line, an unknown name, or a name given more than once;
///   - a value refused by its setting, naming the setting as in the file, e.g.
///     `Could not parse server configuration file '/x/server_environment.cfg': ECF_PORT: invalid value '3141x':
///     expected an integer`.
///
class CfgReader {
public:
    ///
    /// @brief Creates a reader.
    ///
    /// @param[in] catalogue The catalogue of the settings; it must outlive the reader
    ///
    explicit CfgReader(const Catalogue& catalogue);

    ///
    /// @brief Reads a file.
    ///
    /// @param[in] file The file; a missing file is read as an empty file
    /// @return The layer named layers::env_cfg, holding a value for each setting present in the file.
    /// @throws ConfigurationError if the file is refused.
    ///
    [[nodiscard]] Layer read(const std::filesystem::path& file) const;

private:
    const Catalogue& catalogue_;
};

} // namespace ecf::cfg
