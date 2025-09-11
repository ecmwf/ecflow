/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_EcfPortLock_HPP
#define ecflow_core_EcfPortLock_HPP

///
/// \brief This class enables the creation of a lock file, so that different processes
///        avoid creating server with same port number.
///
///        IMPORTANT: This functionality is used in TESTS only.
///

#include <fstream>
#include <iostream>
#include <sstream>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"

namespace ecf {

class EcfPortLock {

    /**
     * Attempts to create the given lock file.
     *
     * \returns false, if: (1) the lock file already exists;
     *                     (2) the lock file could not be created (e.g. due to permissions)
     *          true, if lock file was created successfully
     */
    static bool create_lock_file(const std::string& path) {
        std::cout << " *** Checking if lock file exists: " << path << std::endl;
        if (fs::exists(path)) {
            std::cout << " *** Found an existing lock file! Giving up..." << std::endl;
            return false;
        }
        else {
            std::cout << " *** Didn't find an existing lock file! Creating one NOW..." << std::endl;
            std::ofstream lock(path);
            if (!lock.is_open()) {
                std::cout << " *** Unable to create a lock file! Giving up..." << std::endl;
                return false;
            }
            std::cout << " *** Created lock file: " << path << std::endl;
            return true;
        }
    }

public:
    // Disable default construction
    EcfPortLock() = delete;
    // Disable copy (and move) semantics
    EcfPortLock(const EcfPortLock&)                  = delete;
    const EcfPortLock& operator=(const EcfPortLock&) = delete;

    /**
     * Attempts to lock the given port
     *
     * Has the side-effect of creating a lock file on the filesystem related to the given port
     *
     * \returns true, if lock was achieved; false otherwise.
     */
    static bool try_port_lock(int port, bool debug = false) {
        std::string the_port = ecf::convert_to<std::string>(port);
        std::string the_file = port_file(the_port);
        std::string error;
        if (debug) {
            std::cout << "  EcfPortLock::try_port_lock(" << port << "), creating file: " << the_file;
        }
        if (create_lock_file(the_file)) {
            if (debug) {
                std::cout << "  EcfPortLock::try_lock(" << port << "), got a lock! returning TRUE\n ";
            }
            return true;
        }
        if (debug) {
            std::cout << "  EcfPortLock::try_lock(" << port << "), did NOT get a lock! returning FALSE\n ";
        }
        return false;
    }

    /**
     * Iteratively attempts to lock an available port, starting the search from the given port
     *
     * Has the side-effect of creating a lock file on the filesystem related to the given port
     *
     * \returns the available port
     */
    static int try_next_port_lock(int port, bool debug = false) {
        while (!try_port_lock(port, debug)) {
            ++port;
        }
        return port;
    }

    /**
     * Performs the unlock the the given port
     *
     * Effectively removes the lock file related to the given port
     */
    static void try_port_unlock(int port, bool debug = false) {
        std::string the_port = ecf::convert_to<std::string>(port);
        std::string the_file = port_file(the_port);
        fs::remove(the_file);
    }

    static bool is_free(int port, bool debug = false) {
        std::string the_port = ecf::convert_to<std::string>(port);
        if (fs::exists(port_file(the_port))) {
            if (debug) {
                std::cout << "  EcfPortLock::is_free(" << port << ") returning FALSE\n ";
            }
            return false;
        }
        if (debug) {
            std::cout << "  EcfPortLock::is_free(" << port << ") returning TRUE\n ";
        }
        return true;
    }

    static void create(const std::string& the_port) {
        std::string the_file = port_file(the_port);
        std::string errorMsg;
        if (!ecf::File::create(the_file, "", errorMsg)) {
            std::stringstream sb;
            sb << "EcfPortLock::create_free_port_file : could not create file " << the_file;
            throw std::runtime_error(sb.str());
        }
    }

    static void remove(const std::string& the_port) {
        std::string the_file = port_file(the_port);
        fs::remove(the_file);
    }

private:
    static std::string port_file(const std::string& the_port) {
        // We need the *SAME* location so that different process find the same file.
        // When going across compiler the root_build_dir is not sufficient
        std::string path = File::root_source_dir();
        ecf::environment::get("ECF_PORT_LOCK_DIR", path);

        path += "/";
        path += the_port;
        path += ".lock";

        return path;
    }
};

} // namespace ecf

#endif /* ecflow_core_EcfPortLock_HPP */
