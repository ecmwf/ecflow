/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_test_foolproof_scaffold_Process_HPP
#define ecflow_test_foolproof_scaffold_Process_HPP

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

    int wait();
    int terminate();

    bool is_running() const;

    std::string read_stdout() const;
    std::string read_stderr() const;

    ~Process();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ecf::test::scaffold

#endif /* ecflow_test_foolproof_scaffold_Process_HPP */
