/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/core/File_r.hpp"

using namespace ecf;

File_r::File_r(const std::string& file_name)
    : file_name_(file_name),
      fp_(file_name.c_str(), std::ios_base::in) {
}

File_r::~File_r() {
    fp_.close();
}
