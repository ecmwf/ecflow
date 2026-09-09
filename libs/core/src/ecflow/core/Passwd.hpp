/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_Passwd_HPP
#define ecflow_core_Passwd_HPP

///
/// The tasks send by the ECF will have GENERATED PASSWORD that is not
/// stored into any file (except the job file). It only exists in the
/// ecFlow server memory. If the ecFlow server fails and a new one is started,
/// it must be told (by operator) to accept the messages from the unknown jobs.
///
/// The GENERATED PASSWORDs are stored into the CHECKPOINT file. So if the
/// ECF fails and is RESTARTed from a valid CHECKPOINT file, the ECF gets
/// to know about the tasks.
///
/// The user passwords are kept in a file. Whenever a user logs into
/// the ECF or changes the "level" of privileges while already in the
/// file is consulted.
///

#include <string>

class Passwd {
public:
    // Disable default construction
    Passwd() = delete;

    /// generate a random password
    static std::string generate();
};

#endif /* ecflow_core_Passwd_HPP */
