/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_parser_test_TemporaryFile_HPP
#define ecflow_node_parser_test_TemporaryFile_HPP

#include <string>

#include "ecflow/core/Filesystem.hpp"

class TemporaryFile {
public:
    TemporaryFile();
    explicit TemporaryFile(const std::string& pattern);

    ~TemporaryFile();

    [[nodiscard]] inline std::string path() const { return path_.string(); }
    [[nodiscard]] inline size_t size() const { return fs::file_size(path_); }

private:
    fs::path path_;
};

#endif /* ecflow_node_parser_test_TemporaryFile_HPP */
