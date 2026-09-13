/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_test_scaffold_EcfPortLock_HPP
#define ecflow_test_scaffold_EcfPortLock_HPP

#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/asio.hpp>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/test/scaffold/LockFile.hpp"

namespace ecf::test::scaffold {

///
/// @brief Reserves server port numbers across test processes, by means of port lock files.
///
/// This is a stateless facade over LockFile, retained for test harnesses that track the reserved port as a plain
/// value and release it explicitly with remove(); the lock file is not removed automatically. New code is expected
/// to use MakePort, which releases the lock automatically.
///
class EcfPortLock {
public:
    EcfPortLock() = delete;

    ///
    /// @brief Attempts to lock the given port, by creating the related lock file.
    ///
    /// @param[in] port The port to lock
    /// @param[in] debug Whether to trace the attempt on standard output
    /// @return true if the lock was acquired; false otherwise
    ///
    static bool try_port_lock(int port, bool debug = false) {
        if (debug) {
            std::cout << "  EcfPortLock::try_port_lock(" << port << "), creating file: " << port_file(port);
        }
        if (auto lock = LockFile::make_lock(port_file(port)); lock.has_value()) {
            lock->release();
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

    ///
    /// @brief Locks the first available port, searching upwards from the given port.
    ///
    /// @param[in] port The port to start the search from
    /// @param[in] debug Whether to trace the attempts on standard output
    /// @return The locked port
    ///
    static int try_next_port_lock(int port, bool debug = false) {
        while (!try_port_lock(port, debug)) {
            ++port;
        }
        return port;
    }

    ///
    /// @brief Unlocks the given port, by removing the related lock file.
    ///
    /// @param[in] port The port to unlock
    ///
    static void try_port_unlock(int port, bool /*debug*/ = false) { fs::remove(port_file(port)); }

    ///
    /// @brief Checks whether the given TCP port can be bound on the local machine.
    ///
    /// @param[in] port The TCP port number to check
    /// @return true if the port is free; false if it is occupied or if any error occurs during the check
    ///
    static bool is_tcp_port_free(unsigned short port) {
        using namespace boost::asio;

        io_context io;
        ip::tcp::acceptor a(io);

        boost::system::error_code ec;
        a.open(ip::tcp::v4(), ec);
        if (ec) {
            // If a socket cannot be opened, the port is assumed to be in use.
            std::cout << "  EcfPortLock::is_port_free(" << port << ") : FALSE (unable to open socket)\n ";
            return false;
        }
        a.bind({ip::tcp::v4(), port}, ec);
        if (ec) {
            if (ec == error::address_in_use) {
                std::cout << "  EcfPortLock::is_port_free(" << port
                          << ") : FALSE (unable to bind, due to port already in use)\n ";
            }
            else {
                std::cout << "  EcfPortLock::is_port_free(" << port
                          << ") : FALSE (unable to bind, due to unknown issue)\n ";
            }
            return false;
        }
        return true;
    }

    ///
    /// @brief Checks whether the given port is neither locked (by lock file) nor bound (by a running process).
    ///
    /// @param[in] port The port to check
    /// @param[in] debug Whether to trace the checks on standard output
    /// @return true if the port is free; false otherwise
    ///
    static bool is_free(int port, bool debug = false) {
        // 1. File-lock check (fast path)
        if (fs::exists(port_file(port))) {
            if (debug) {
                std::cout << "  EcfPortLock::is_free(" << port << ") returning FALSE (lock file exists)\n ";
            }
            return false;
        }

        // 2. TCP socket check
        // This ensures the port is free, by actually trying to bind to it (and immediately releasing it).
        if (!is_tcp_port_free(port)) {
            if (debug) {
                std::cout << "  EcfPortLock::is_free(" << port << ") returning FALSE (TCP port occupied)\n ";
            }
            return false;
        }
        if (debug) {
            std::cout << "  EcfPortLock::is_free(" << port << ") returning TRUE\n ";
        }
        return true;
    }

    ///
    /// @brief Ensures the lock file for the given port exists.
    ///
    /// An already existing lock file is accepted, since callers commonly re-affirm a lock obtained earlier
    /// (for example, by try_next_port_lock()).
    ///
    /// @param[in] the_port The port to lock, as text
    /// @throws std::runtime_error if the lock file does not exist and cannot be created
    ///
    static void create(const std::string& the_port) {
        auto the_file = LockFile::port_lock_path(the_port);
        if (fs::exists(the_file)) {
            return;
        }
        if (auto lock = LockFile::make_lock(the_file); lock.has_value()) {
            lock->release();
            return;
        }
        throw std::runtime_error(MESSAGE("EcfPortLock::create : could not create file " << the_file));
    }

    ///
    /// @brief Removes the lock file for the given port.
    ///
    /// @param[in] the_port The port to unlock, as text
    ///
    static void remove(const std::string& the_port) { fs::remove(LockFile::port_lock_path(the_port)); }

private:
    static fs::path port_file(int port) { return LockFile::port_lock_path(ecf::convert_to<std::string>(port)); }
};

} // namespace ecf::test::scaffold

#endif /* ecflow_test_scaffold_EcfPortLock_HPP */
