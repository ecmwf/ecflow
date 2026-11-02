// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <string>

#include "ecflow/core/cfg/Catalogue.hpp"
#include "ecflow/core/cfg/Configuration.hpp"

namespace ecf::cfg {

///
/// @brief Reads the options of the command line into a layer of settings of a catalogue.
///
/// Each setting with the name of an option (Names::cli) takes the text of that option, when it is given, turned into
/// a value of the setting by Setting::parse(). The syntax of the command line is checked before, by its parser.
///
/// The reader refuses a value refused by its setting, naming the option, e.g.
/// `Invalid server command line: --port: invalid value '80': expected a value in [1024, 49150]`.
///
class CliReader {
public:
    ///
    /// @brief Creates a reader.
    ///
    /// @param[in] catalogue The catalogue of the settings; it must outlive the reader
    ///
    explicit CliReader(const Catalogue& catalogue);

    ///
    /// @brief Reads the options given.
    ///
    /// @param[in] options The options given, by name without the leading dashes, each with its text; the text of a
    /// switch is empty, and an option that names no setting is ignored
    /// @return The layer named layers::cli, holding a value for each setting whose option is given.
    /// @throws ConfigurationError if a value is refused.
    ///
    [[nodiscard]] Layer read(const std::map<std::string, std::string>& options) const;

private:
    const Catalogue& catalogue_;
};

} // namespace ecf::cfg
