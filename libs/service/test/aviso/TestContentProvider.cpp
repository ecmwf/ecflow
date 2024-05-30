/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TestContentProvider.hpp"

#include <array>
#include <cstdio>
#include <fstream>

#include <boost/filesystem.hpp>

namespace ecf::test {

namespace {

std::string make_temp_filename(std::string file_name_prefix) {
    file_name_prefix += "XXXXXX";
    std::array<char, 4096> file_name_template{};
    std::copy(file_name_prefix.begin(), file_name_prefix.end(), file_name_template.data());
    mkstemp(file_name_template.data());
    return file_name_prefix;
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
    boost::filesystem::remove(file_path);
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
