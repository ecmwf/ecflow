/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_harness_TestFixture_HPP
#define ecflow_test_harness_TestFixture_HPP

///
/// \brief This Fixture facilitates the test of client/server on different platforms
///
/// In order to carry out the test, we must have a common file system.
/// We will use $SCRATCH as this is accessible by both client and server.
/// This means copying over the test data
///
/// When TestFixture is GLOBAL, then we can't seem to call any of the
/// BOOST_REQUIRE_MESSAGE() macro in constructor/descructor as this causes a crash
/// i.e order of initialisation issues
///

#include <string>

#include "ScratchDir.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/PrintStyle.hpp"

class Defs;

//
// Need to use static data, since with boost global fixture, its not possible to access
// the global test fixture in each of the test cases
//
struct TestFixture
{

    // Constructor will invoke the server, destructor will kill the server
    // Since this class is static, the constructor/destructor cannot call
    // any of BOOST MACRO, since the unit test will not be there.
    // When running across platforms will will assume server is already running
    explicit TestFixture(const std::string& project_test_dir /* Test or view */);
    TestFixture();
    ~TestFixture();

    static int job_submission_interval();

    /// The location of ECF home will vary. If client/server on same machines we
    /// return test data location. Otherwise we need return a common file system location
    /// that was created in the constructor
    static std::string smshome();

    /// If running locally returns  location of client exe, if a server is on a remote
    /// machine, we need to determine its location.
    // Several options:
    //    a/ Search for hard code path
    //    b/ Ask server about test data, i.e. client exe path, log file location , etc
    //       More flexible
    static std::string theClientExePath();

    /// When multiple tests are run , we need to clear the log file
    static void clearLog();

    /// When local just returns ecf.log, when remote return path to log file
    static std::string pathToLogFile();

    /// Given a task name like "a" find the first task matching that name
    /// and returns is abs node path
    static std::string taskAbsNodePath(const Defs& theDefs, const std::string& taskName);

    /// Location of the includes used in the ecf file
    static std::string includes();

    /// Retrieve the server version
    ///
    /// This allows to ignore some tests, which is useful when testing old servers using new clients
    ///
    static const std::string& server_version();

    // Use for all comms with server
    static ClientInvoker& client();
    static const std::string& port() { return port_; }

private:
    static std::string local_ecf_home();

    void init(const std::string& project_test_dir);

private:
    static std::unique_ptr<ScratchDir> scratch_dir_;
    static std::string host_;
    static std::string port_;
    static std::string test_dir_;         // used when we have an external server, different platform
    static std::string project_test_dir_; // "Test" or "view"
    PrintStyle print_style_;              // by default show state when writing defs to standard out. RAII
};

#endif /* ecflow_test_harness_TestFixture_HPP */
