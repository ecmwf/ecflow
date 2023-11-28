/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TemporaryFile.hpp"

TemporaryFile::TemporaryFile() : path_{fs::unique_path("tmp_%%%%-%%%%-%%%%-%%%%")} {
}

TemporaryFile::TemporaryFile(const std::string& pattern) : path_{fs::unique_path(pattern)} {
}

TemporaryFile::~TemporaryFile() {
    try {
        fs::remove(path_);
    }
    catch (...) {
        // Nothing to do...
    }
}
