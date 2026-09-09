/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_TimeStamp_HPP
#define ecflow_core_TimeStamp_HPP

#include <string>

namespace ecf {

///
/// @brief Utilities for generating formatted wall-clock timestamps.
///
namespace TimeStamp {

///
/// @brief Return a timestamp string representing the current time.
///
/// Format: `"[HH:MM:SS D.M.YYYY] "` (e.g. `"[05:26:20 29.10.2014] "` or `"[05:26:20 17.1.2023] "`).
/// Note that day and month have no leading zeros.
///
/// @return A formatted timestamp string.
///
std::string now();

///
/// @brief Append a timestamp of the current time to \p buffer.
///
/// Format: `"[HH:MM:SS D.M.YYYY] "` (e.g. `"[05:26:20 29.10.2014] "` or `"[05:26:20 17.1.2023] "`).
/// Note that day and month have no leading zeros.
///
/// @param buffer The string to which the formatted timestamp is appended.
///
void now(std::string& buffer);

///
/// @brief Append a brief timestamp of the current time to \p buffer.
///
/// The brief format omits the year component.
/// Format: `"[HH:MM:SS D.M] "` (e.g. `"[05:26:20 29.10] "` or `"[05:26:20 17.1] "`).
/// Note that day and month have no leading zeros.
///
/// @param buffer The string to which the formatted timestamp is appended.
///
void now_in_brief(std::string& buffer);

} // namespace TimeStamp
} // namespace ecf

#endif /* ecflow_core_TimeStamp_HPP */
