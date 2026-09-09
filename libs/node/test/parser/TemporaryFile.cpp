/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TemporaryFile.hpp"

TemporaryFile::TemporaryFile()
    : path_{ecf::fsx::unique_path("tmp_%%%%-%%%%-%%%%-%%%%")} {
}

TemporaryFile::TemporaryFile(const std::string& pattern)
    : path_{ecf::fsx::unique_path(pattern)} {
}

TemporaryFile::~TemporaryFile() {
    try {
        fs::remove(path_);
    }
    catch (...) {
        // Nothing to do...
    }
}
