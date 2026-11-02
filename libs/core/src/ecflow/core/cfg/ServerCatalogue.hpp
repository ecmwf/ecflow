// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/core/cfg/Catalogue.hpp"

namespace ecf::cfg {

///
/// @brief Returns the catalogue of the inputs of the server.
///
/// The catalogue describes every input of the server available in develop, under keys of the form:
///
///   - `ecflow.server.variables@<NAME>`:  an input with an `ECF_` name, e.g. `ecflow.server.variables@ECF_PORT`;
///   - `ecflow.server.<path>`:            an input available only on the command line, e.g. `ecflow.server.http`.
///
/// Notes:
///
///   - Every text setting is required to be non-empty, except `ECF_SSL`.
///   - `ECF_DEBUG_SERVER` enables debug whenever it is defined, whatever its text; `--debug` enables it too.
///   - The command-line switches take no text: `--http`, `--v6` and `--ssl` set their setting to `true`, and
///     `--dis_job_gen` sets `ecflow.server.job_generation` to `false`.
///
/// @return The catalogue, created on first use.
///
const Catalogue& server_catalogue();

} // namespace ecf::cfg
