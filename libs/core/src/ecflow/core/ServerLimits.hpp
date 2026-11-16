// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

namespace ecf {

///
/// @brief Holds the limits on the inputs of a server (R11.10), set in its configuration file under
///        `/ecflow/server/limits`.
///
/// Each limit is enforced where the input arrives; 0 stands for no limit.
///
struct ServerLimits
{
    std::size_t request_size{0};        ///< The largest request accepted, in bytes (its serialised form)
    std::size_t permissions_entries{0}; ///< The largest number of entries of a Permissions attribute brought in
    std::size_t handles_per_user{0};    ///< The largest number of client handles a user holds
};

} // namespace ecf
