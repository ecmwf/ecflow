/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
