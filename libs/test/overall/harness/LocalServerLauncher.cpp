/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LocalServerLauncher.hpp"

#include <iostream>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"

void LocalServerLauncher::launch() {
    // (1) Clear previous execution artifacts, before launching the server
    ecf::Host h{host_};
    // Remove the log file, to ensure we start with a clean slate
    fs::remove(h.ecf_log_file(port_));
    // Remove the check point files, otherwise server will load check point file
    fs::remove(h.ecf_checkpt_file(port_));
    fs::remove(h.ecf_backup_checkpt_file(port_));

    // (2) Build the command to launch the server
    std::string cmd = ecf::File::find_ecf_server_path();
    cmd += " -d";
    cmd += " --port " + port_;
    cmd += use_http_ ? " --http" : "";
    cmd += " --ecfinterval=" + ecf::convert_to<std::string>(submission_interval_);
    cmd += " &";

    // (3) Launch the server
    int exit = system(cmd.c_str());
    assert(exit == 0);

    std::cout << "   Local ecFlow server launched with command: '" << cmd << "'. Exit code: " << exit << std::endl;
}
