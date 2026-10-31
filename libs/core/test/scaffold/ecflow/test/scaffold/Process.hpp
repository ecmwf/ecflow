// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace ecf::test::scaffold {

///
/// @brief Launches a child process for a test, and owns its exit status and captured output.
///
/// The object owns at most one child. Ownership moves with the object; a default-constructed or moved-from object
/// owns none.
///
/// @invariant The child is reaped at most once; its exit status is cached and returned by every later wait().
///
class Process {
public:
    ///
    /// @brief Creates an object that owns no child.
    ///
    Process();

    ///
    /// @brief Launches the given executable as an owned child, capturing its standard output and error.
    ///
    /// @param[in] executable Path of the program to run
    /// @param[in] args Command-line arguments, without the program name
    /// @param[in] cwd Working directory of the child
    /// @throws std::exception if the child cannot be launched.
    ///
    Process(const fs::path& executable, std::vector<std::string> args, fs::path cwd = fs::current_path());

    Process(const Process& rhs)            = delete;
    Process& operator=(const Process& rhs) = delete;

    ///
    /// @brief Takes over the child owned by another object, which then owns none.
    ///
    /// @param[in,out] rhs Object whose child is taken over
    ///
    Process(Process&& rhs);

    ///
    /// @brief Takes over the child owned by another object, which then owns none.
    ///
    /// A child previously owned by this object is released without being terminated or reaped.
    ///
    /// @param[in,out] rhs Object whose child is taken over
    /// @return This object.
    ///
    Process& operator=(Process&& rhs);

    ///
    /// @brief Returns the process id of the owned child.
    ///
    /// @return The process id, which stays reserved until the child is reaped.
    /// @throws std::logic_error if no child is owned (default-constructed or moved-from object).
    ///
    int pid() const;

    ///
    /// @brief Reaps the child once and returns its cached exit status on subsequent calls.
    ///
    /// Concurrent wait and termination requests share a single native waiter.
    ///
    /// @return The child's exit status.
    /// @throws std::logic_error if no child is owned (default-constructed or moved-from object).
    /// @throws std::exception if the native wait fails.
    ///
    int wait();

    ///
    /// @brief Terminates and reaps the owned child without throwing or reaping it twice.
    ///
    /// An already reaped child is left untouched. Concurrent wait requests receive the same exit status.
    ///
    /// @return Zero on success, or one if signalling or waiting fails.
    ///
    int terminate() noexcept;

    ///
    /// @brief Reports whether a child is owned and has not yet been reaped.
    ///
    /// @return False for a default-constructed or moved-from object, and once the child has been waited for.
    ///
    bool is_running() const;

    ///
    /// @brief Reads the standard output of the owned child until the child closes it.
    ///
    /// Requires an owned child; call it after wait() or terminate(), since it blocks while the child runs.
    ///
    /// @return The output not yet read.
    ///
    std::string read_stdout() const;

    ///
    /// @brief Reads the standard error of the owned child until the child closes it.
    ///
    /// Requires an owned child; call it after wait() or terminate(), since it blocks while the child runs.
    ///
    /// @return The output not yet read.
    ///
    std::string read_stderr() const;

    ///
    /// @brief Releases the object without terminating or reaping the child; call terminate() or wait() first.
    ///
    ~Process();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ecf::test::scaffold
