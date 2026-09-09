/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TestContentProvider.hpp"

#include <array>
#include <cstdio>
#include <fstream>

#include "ecflow/core/Filesystem.hpp"

namespace ecf::test {

namespace {

std::string make_temp_filename(std::string file_name_prefix) {
    file_name_prefix += "XXXXXX";
    std::array<char, 4096> file_name_template{};
    std::copy(file_name_prefix.begin(), file_name_prefix.end(), file_name_template.data());
    mkstemp(file_name_template.data());
    return file_name_template.data();
}

void store_content_to_file(const std::string& file_path, const std::string& content) {
    std::ofstream file(file_path);
    if (file.is_open()) {
        file << content;
    }
    else {
        throw std::runtime_error("Failed to open temporary file: " + file_path);
    }
}

void clear_content_from_file(const std::string& file_path) {
    std::filesystem::remove(file_path);
}

} // namespace

TestContentProvider::TestContentProvider(const std::string& file_name_prefix) {
    file_ = make_temp_filename(file_name_prefix);
    store_content_to_file(file_, "");
}

TestContentProvider::TestContentProvider(const std::string& file_name_prefix, const std::string& content) {
    file_ = make_temp_filename(file_name_prefix);
    store_content_to_file(file_, content);
}

TestContentProvider::~TestContentProvider() {
    clear_content_from_file(file_);
}

} // namespace ecf::test
