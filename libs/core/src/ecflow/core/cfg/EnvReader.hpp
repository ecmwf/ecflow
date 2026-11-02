// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/core/cfg/Catalogue.hpp"
#include "ecflow/core/cfg/Configuration.hpp"

namespace ecf::cfg {

///
/// @brief Reads the environment variables into a layer of settings of a catalogue.
///
/// Each setting with the name of an environment variable (Names::env) takes the value of that variable, when it is
/// defined, turned into a value of the setting by Setting::parse().
///
/// The reader refuses a value refused by its setting, naming the variable, e.g.
/// `Environment variable ECF_PORT: invalid value '80': expected a value in [1024, 49150]`.
///
class EnvReader {
public:
    ///
    /// @brief Creates a reader.
    ///
    /// @param[in] catalogue The catalogue of the settings; it must outlive the reader
    ///
    explicit EnvReader(const Catalogue& catalogue);

    ///
    /// @brief Reads the environment variables.
    ///
    /// @return The layer named layers::env, holding a value for each setting whose variable is defined.
    /// @throws ConfigurationError if a value is refused.
    ///
    [[nodiscard]] Layer read() const;

private:
    const Catalogue& catalogue_;
};

} // namespace ecf::cfg
