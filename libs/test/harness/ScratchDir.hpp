/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_harness_ScratchDir_HPP
#define ecflow_test_harness_ScratchDir_HPP

#include <cassert>
#include <string>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"

/**
 * This class is used to create a scratch directory for testing purposes.
 *
 * The scratch directory location is based on the $SCRATCH environment variable,
 * and the must be accessible by the test processes.
 */
class ScratchDir {
public:
    /**
     * Create a scratch directory for testing purposes.
     *
     * Throws an EnvVarNotFound exception if:
     *  - the $SCRATCH environment variable is not defined
     *  - the $SCRATCH environment variable is empty
     */
    ScratchDir()
        : base_dir_(get_scratch_base_dir()),
          test_dir_(base_dir_ + "/test_dir"),
          home_dir_(test_dir_ + "/ECF_HOME") {
        clear_scratch_test_dir(test_dir_);
    }
    ~ScratchDir() { clear_scratch_test_dir(test_dir_); }

    const std::string& home_dir() const { return home_dir_; }
    const std::string& test_dir() const { return test_dir_; }

private:
    std::string base_dir_;
    std::string test_dir_;
    std::string home_dir_;

    inline static const char* SCRATCH = "SCRATCH";

    inline static std::string get_scratch_base_dir() {
        auto scratch = ecf::environment::get(SCRATCH);
        if (scratch.empty()) {
            throw ecf::environment::EnvVarNotFound(ecf::Message(SCRATCH).str());
        }
        return scratch;
    }

    inline static void clear_scratch_test_dir(const std::string& test_dir) {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
};

#endif /* ecflow_test_harness_ScratchDir_HPP */
