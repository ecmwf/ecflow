/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TestFixture.hpp"

#include <fstream> // for ofstream
#include <iostream>

#include "LocalServerLauncher.hpp"
#include "SCPort.hpp"
#include "TestHelper.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/client/ClientEnvironment.hpp" // needed for static ClientEnvironment::hostSpecified(); ONLY
#include "ecflow/client/Rtt.hpp"
#include "ecflow/core/EcfPortLock.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Task.hpp"

#ifdef DEBUG
std::string rtt_filename = "rtt.dat";
#else
std::string rtt_filename = "rtt_d.dat";
#endif
std::unique_ptr<ScratchDir> TestFixture::scratch_dir_;
std::string TestFixture::host_;
std::string TestFixture::port_;
std::string TestFixture::test_dir_;
std::string TestFixture::project_test_dir_ = "libs/test";

using namespace std;
using namespace ecf;

namespace /* anonymous */ {

bool is_external_server_running_remotelly(std::string_view host) {
    return !host.empty() && host != Str::LOCALHOST();
}

bool is_external_server_running_locally(std::string_view host) {
    return !host.empty() && host == Str::LOCALHOST();
}

bool is_local_server(std::string_view host) {
    return host.empty() || host == Str::LOCALHOST();
}

} // namespace

// ************************************************************************************************
// For test purpose the server can be started:
//
//  (1) Externally but on a different platform. by defining env variable: export ECF_HOST=itanium
//      In this case we NEED to copy the test data, so that it is
//      accessible by the client AND server
//  (2) Externally but on the same machine. by defining env variable: export ECF_HOST=localhost
//      WHY? To test for memory leak. (i.e. with valgrind)
//  (3) By this test fixture
//
//  When invoking the server Externally _MUST_ use   .
//     ./Server/bin/gcc.<version>/debug/server --ecfinterval=2
//        The --ecfinterval=2 is _important_ or test will take a long time
// ************************************************************************************************

// Uncomment to preserve the files for test
// #define DEBUG_HOST_SERVER 1
#define DEBUG_LOCAL_SERVER 1

TestFixture::TestFixture(const std::string& project_test_dir) {
    init(project_test_dir);
}

TestFixture::TestFixture() {
    init("libs/test");
}

ClientInvoker& TestFixture::client() {
    static ClientInvoker theClient_;
    return theClient_;
}

void TestFixture::init(const std::string& project_test_dir) {
    TestFixture::project_test_dir_ = project_test_dir;
    auto test_dir                  = local_ecf_home();
    std::cout << "TestFixture::TestFixture() project_test_dir      :" << project_test_dir << "\n";
    std::cout << "TestFixture::TestFixture() local_ecf_home        :" << test_dir << "\n";
    std::cout << "TestFixture::TestFixture() cwd                   :" << fs::current_path() << "\n";

    if (!fs::exists(test_dir)) {
        fs::create_directories(test_dir);
    }

    // client side file for recording all ClientInvoker round trip times
    fs::remove(rtt_filename);
    Rtt::create(rtt_filename);

    // ********************************************************
    // Note: Global fixture Constructor cannot use BOOST macro
    // ********************************************************

    // Let first see if we need do anything. If ECF_HOST is specified (ie the name
    // of the machine, which has the ecflow server), only then do we need to do anything.
    // The server must have access to the file system specified by ECF_HOME.
    // This becomes an issue when the server is on a different machine
    host_ = ClientEnvironment::hostSpecified();
    port_ = ClientEnvironment::portSpecified(); // returns ECF_PORT, otherwise Str::DEFAULT_PORT_NUMBER
#ifdef ECF_OPENSSL
    if (ecf::environment::has("ECF_SSL")) {
        std::cout << "   Openssl enabled\n";
    }
#endif
    if (is_external_server_running_remotelly(host_)) {

        // Option (1)
        //
        // Going to perform the tests using an external server, running on a remote machine.
        // We require the external server file system to be mounted locally,
        // and use the $SCRATCH environment variable to locate the mount point of the server data.
        // This allows to deploy the necessary test data, reset ECF_HOME, and configure the server.

        client().set_host_port(host_, port_);

        std::cout << "   _EXTERNAL_ SERVER running on a _REMOTE_ platform";
        std::cout << " (" << host_ << ":" << port_ << ").\n";

        scratch_dir_ = std::make_unique<ScratchDir>();

        if (const auto& d = scratch_dir_->test_dir(); fs::exists(d)) {
            fs::remove_all(d);
        }

        {
            auto success = File::createDirectories(scratch_dir_->home_dir());
            BOOST_REQUIRE_MESSAGE(success, "Create the home directory inside the $SCRATCH");
        }
        {
            auto success = fs::exists(scratch_dir_->home_dir());
            BOOST_REQUIRE_MESSAGE(success, "The home directory inside the $SCRATCH exists");
        }
        { // Ensure that local includes data exists, before attempting to copy it into $SCRATCH
            auto success = fs::exists(includes());
            BOOST_REQUIRE_MESSAGE(success, "The includes directory exists");
        }

        std::cout << " Copying test data ...\n";
        // Copy over the includes directory to the SCRATCH area.
        std::string scratchIncludes = scratch_dir_->test_dir() + "/";
        std::string do_copy         = "cp -r " + includes() + " " + scratchIncludes;
        if (system(do_copy.c_str()) != 0) {
            assert(false);
        }

        // clear log file
        clearLog();
    }
    else if (is_external_server_running_locally(host_)) {

        // Option (2)
        //
        // Going to perform the tests using an external server, running on the local machine.

        client().set_host_port(host_, port_);

        std::cout << "   _EXTERNAL_ SERVER running on the _LOCAL_ platform";
        std::cout << " (" << client().host() << ":" << client().port() << ").\n";

        // Print the server stats before we start + checks if it is up and running::
        client().set_cli(true); // so server stats are written to standard out
        if (client().stats() != 0) {
            std::cout << "   ClientInvoker " << CtsApi::stats() << " failed: " << client().errorMsg()
                      << ". Is the server running?\n";
            assert(false);
        }
        client().set_cli(false);

        // log file may have been deleted, by previous tests. Create a new log file
        std::string the_log_file = TestFixture::pathToLogFile();
        if (!fs::exists(the_log_file)) {
            std::cout << "   Log file " << the_log_file << " does NOT exist, attempting to recreate\n";
            std::cout << "   Creating new log(via remote server) file " << the_log_file << "\n";
            if (0 == client().new_log(the_log_file)) {
                client().logMsg("Created new log file. msg sent to force new log file to be written to disk");
            }
            else
                cout << "   Log file " << TestFixture::pathToLogFile() << " creation failed " << client().errorMsg()
                     << "\n";
        }
        else {
            cout << "   Log file " << the_log_file << " already exists\n";
        }
    }
    else {

        // Option (3)
        //
        // Going to perform the tests using a server launched as part of the test setup.

        // Update ClientInvoker with local host and port
        host_ = Str::LOCALHOST();
        port_ = ecf::SCPort::find_available_port(port_);
        client().set_host_port(host_, port_);

        std::cout << "   _LOCAL_ SERVER running on the _LOCAL_ platform";
        std::cout << " (" << host_ << ":" << port_ << ").\n";

        bool use_http = false;
        if (auto found = ecf::environment::fetch("ECF_TEST_USING_HTTP"); found) {
            use_http = found.value() == "1";
            client().enable_http();
            client().debug(true);
        }

        // clang-format off
        LocalServerLauncher{}
            .with_host(host_)
            .with_port(port_)
            .using_http(use_http)
            .launch();
        // clang-format on
    }

    /// Ping the server to see if its running
    /// Assume remote/local server started on the default port
    /// Either way, we wait for 60 seconds for server, for it to respond to pings
    /// This is important when server is started locally. We must wait for it to come alive.
    if (!client().wait_for_server_reply()) {
        cout << "   Ping server on " << client().host() << Str::COLON() << client().port()
             << " failed. Is the server running ? " << client().errorMsg() << "\n";
        assert(false);
    }
    cout << "   Ping OK: server running on:  " << client().host() << Str::COLON() << client().port() << "\n";

    // Log file must exist, otherwise test will not work. Log file required for comparison
    if (!fs::exists(TestFixture::pathToLogFile())) {
        cout << "   Log file " << TestFixture::pathToLogFile() << " does not exist *************************** \n";
        assert(false);
    }

    // Check host and port match
    assert(host_ == client().host());
    assert(port_ == client().port());
}

TestFixture::~TestFixture() {
    // Note: Global fixture Destructor cannot use BOOST macro
    std::cout << "TestFixture::~TestFixture() " << client().host() << ":" << client().port() << "\n";

    // destructors should not allow exception propagation
    try {
#ifndef DEBUG_HOST_SERVER
        if (!host_.empty() && fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
#endif
#ifndef DEBUG_LOCAL_SERVER
        if (fs::exists(local_ecf_home())) {
            fs::remove_all(local_ecf_home());
        }
#endif

        std::cout << "   Print the server suites\n";
        client().set_cli(true);             // so server stats are written to standard out
        client().set_throw_on_error(false); // destructors should not allow exception propagation
        if (client().suites() != 0) {
            std::cout << "TestFixture::~TestFixture(): ClientInvoker " << CtsApi::suites()
                      << " failed: " << client().errorMsg() << "\n";
        }

        std::cout << "   Print the server stats\n";
        if (client().stats() != 0) {
            std::cout << "TestFixture::~TestFixture(): ClientInvoker " << CtsApi::stats()
                      << " failed: " << client().errorMsg() << "\n";
        }

        std::cout << "   Kill the server, as all suites are complete. will work for local or external\n";
        if (client().terminateServer() != 0) {
            std::cout << "TestFixture::~TestFixture():  ClientInvoker " << CtsApi::terminateServer()
                      << " failed: " << client().errorMsg() << "\n";
            EcfPortLock::remove(port_);
            assert(false);
        }
        sleep(1); // allow time to update log file

        std::cout << "   Remove the generated check point files, at end of test\n";
        Host host;
        fs::remove(host.ecf_log_file(port_));
        fs::remove(host.ecf_checkpt_file(port_));
        fs::remove(host.ecf_backup_checkpt_file(port_));

        std::cout << "   remove the lock file\n";
        EcfPortLock::remove(port_);

        std::cout << "   Rtt::destroy(), so that we flush the rtt_filename\n";
        Rtt::destroy();

        cout << "\nTiming: *NOTE*: The child commands *NOT* recorded. Since its a separate exe(ecflow_client), called "
                "via .ecf script\n";
        cout << Rtt::analysis(rtt_filename); // report round trip times
        fs::remove(rtt_filename);
    }
    catch (std::exception& ex) {
        std::cout << "TestFixture::~TestFixture() caught exception " << ex.what() << "\n";
    }
    catch (...) {
        std::cout << "TestFixture::~TestFixture() caught unknown exception\n";
    }

    std::cout << "TestFixture::~TestFixture() END\n";
}

int TestFixture::job_submission_interval() {
    return LocalServerLauncher::job_submission_interval();
}

std::string TestFixture::smshome() {
    if (is_local_server(TestFixture::host_)) {
        return local_ecf_home();
    }
    return scratch_dir_->home_dir();
}

std::string TestFixture::theClientExePath() {
    std::string extra_options = "";
    if (auto var = ecf::environment::fetch("ECF_TEST_USING_HTTP"); var) {
        extra_options = " --http";
    }

    if (is_local_server(TestFixture::host_)) {
        return File::find_ecf_client_path() + extra_options;
    }
    else if (auto client_path_p = ecf::environment::fetch("ECF_CLIENT_EXE_PATH"); client_path_p) {
        return client_path_p.value() + extra_options;
    }
    else {
        // Try this before complaining
        std::string path = "/usr/local/apps/ecflow/current/bin/ecflow_client";
        if (fs::exists(path)) {
            return path + extra_options;
        }

        cout << "Please set ECF_CLIENT_EXE_PATH. This needs to be set to path to the client executable\n";
        cout << "The client must be the one that was built on the same platform as the server\n";
        assert(false);
        return string(); // This is needed to silence compiler warnings about no return
    }
}

void TestFixture::clearLog() {

    // Can't remove log on remote server, just clear the log file
    client().clearLog();
}

std::string TestFixture::pathToLogFile() {
    if (is_local_server(TestFixture::host_)) {
        Host host;
        return host.ecf_log_file(port_);
    }

    if (auto var = ecf::environment::fetch("ECF_LOG"); var) {
        return var.value();
    }
    else {
        cout << "TestFixture::pathToLogFile(): assert failed\n";
        cout << "Please set ECF_LOG. This needs to be set to path to the log file\n";
        cout << "that can be seen by the client and server\n";
        assert(false);
        return string(); // This is needed to silence compiler warnings about no return
    }
}

std::string TestFixture::local_ecf_home() {
    std::string compiler = "unknown";
#if defined(_AIX)
    compiler = "aix";
#elif defined(HPUX)
    compiler = "hpux";
#else
    #if defined(__clang__)
    compiler = "clang";
    #elif defined(__INTEL_COMPILER)
    compiler = "intel";
    #elif defined(_CRAYC)
    compiler = "cray";
    #else
    compiler = "gnu";
    #endif
#endif

    std::string build_type = "unknown";
#ifdef DEBUG
    build_type = "debug";
#else
    build_type = "release";
#endif

    auto pid = getpid();
    std::string rel_path =
        "data/ECF_HOME__" + build_type + "__" + compiler + "__pid" + ecf::convert_to<std::string>(pid) + "__";

    // Allow post-fix to be added, to allow test to run in parallel
    if (auto custom_postfix = ecf::environment::fetch("TEST_ECF_HOME_POSTFIX"); custom_postfix) {
        rel_path += custom_postfix.value();
    }

    std::string absolute_path = File::test_data_in_current_dir(rel_path);
    return absolute_path;
}

std::string TestFixture::includes() {
    // Get to the root source directory
    std::string includes_path = File::root_source_dir();
    includes_path += "/";
    includes_path += project_test_dir_;
    includes_path += "/data/includes";
    // std::cout << "includes_path = " << includes_path << " ==============================================\n";
    return includes_path;
}

/// Given a task name like "a" find the first task matching that name and return the absolute node path
std::string TestFixture::taskAbsNodePath(const Defs& theDefs, const std::string& taskName) {
    std::vector<Task*> vec;
    theDefs.getAllTasks(vec);
    for (Task* t : vec) {
        if (t->name() == taskName)
            return t->absNodePath();
    }

    cout << "TestFixture::taskAbsNodePath: assert failed: Could not find task " << taskName << "\n";
    assert(false); // could not find the task ??
    return string();
}

const std::string& TestFixture::server_version() {
    client().server_version();
    return TestFixture::client().get_string();
}
