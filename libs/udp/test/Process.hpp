/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_udp_test_Process_HPP
#define ecflow_udp_test_Process_HPP

#include <memory>
#include <string_view>
#include <vector>

namespace ecf::test {

class Process {
public:
    Process();
    Process(const Process& rhs) = delete;
    Process(Process&& rhs);

    Process(std::string_view executable, std::vector<std::string_view> args);

    Process& operator=(const Process& rhs) = delete;
    Process& operator=(Process&& rhs);

    void terminate();

    ~Process();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ecf::test

#endif /* ecflow_udp_test_Process_HPP */
