// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ecf {

///
/// @brief Records a variable substitution, for the log: the variables used, and the command with its credentials
/// masked.
///
/// Every value of a variable whose name is a credential name (see ecf::is_credential_name()) is replaced by `***`,
/// both in the variables and in the masked command.
///
struct SubstitutionRecord
{
    /// The name and the value used of each variable substituted, in the order of first use, once each; for
    /// `%NAME:default%`, the value of NAME when defined, and the default otherwise
    std::vector<std::pair<std::string, std::string>> variables;
    /// The command as substituted, with `***` in place of the value of each variable with a credential name,
    /// whether referenced directly or through the value of another variable; when the substitution fails, the
    /// command as partially substituted, likewise masked
    std::string masked_command;
};

} // namespace ecf
