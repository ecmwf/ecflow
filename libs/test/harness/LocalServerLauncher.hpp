/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_harness_LocalServerLauncher_HPP
#define ecflow_test_harness_LocalServerLauncher_HPP

#include <cassert>
#include <string>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Str.hpp"

/**
 * This class is used to launch a local test server.
 */
class LocalServerLauncher {
public:
    LocalServerLauncher& with_host(const std::string& host) {
        assert(!host.empty());
        host_ = host;
        return *this;
    };

    LocalServerLauncher& with_port(const std::string& port) {
        assert(!port.empty());
        port_ = port;
        return *this;
    };

    LocalServerLauncher& using_http(bool use_http = true) {
        use_http_ = use_http;
        return *this;
    };

    void launch();

    static int job_submission_interval() {

        // Configure the server with a short job submission interval.
        // For each 'n' seconds of job submission interval the calendar
        // is typically incremented by 1 minute. Hence speeding up the
        // time and thus the testing. See Calendar for further details.

#if defined(HPUX) || defined(_AIX)
        int jobSubmissionInterval = 6; // HPUX and AIX are slow
#else
        int jobSubmissionInterval = 3;
#endif
        return jobSubmissionInterval;
    }

private:
    std::string host_        = ecf::Str::LOCALHOST();
    std::string port_        = default_port();
    bool use_http_           = false;
    int submission_interval_ = job_submission_interval();

    static std::string default_port() { return "3141"; }
};

#endif /* ecflow_test_harness_LocalServerLauncher_HPP */
