/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
