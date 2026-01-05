/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_foolproof_scaffold_Process_HPP
#define ecflow_test_foolproof_scaffold_Process_HPP

#include <memory>
#include <string>
#include <vector>

#include "ecflow/core/Filesystem.hpp"

namespace foolproof::scaffolf {

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

} // namespace foolproof::scaffolf

#endif /* ecflow_test_foolproof_scaffold_Process_HPP */
