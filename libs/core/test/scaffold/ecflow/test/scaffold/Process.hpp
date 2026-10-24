// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace ecf::test::scaffold {

class Process {
public:
    Process();

    Process(const fs::path& executable, std::vector<std::string> args, fs::path cwd = fs::current_path());

    Process(const Process& rhs)            = delete;
    Process& operator=(const Process& rhs) = delete;
    Process(Process&& rhs);
    Process& operator=(Process&& rhs);

    int pid() const;

    ///
    /// @brief Reaps the child once and returns its cached exit status on subsequent calls.
    ///
    /// Concurrent wait and termination requests share a single native waiter.
    ///
    /// @return The child's exit status.
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

    bool is_running() const;

    std::string read_stdout() const;
    std::string read_stderr() const;

    ~Process();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ecf::test::scaffold
