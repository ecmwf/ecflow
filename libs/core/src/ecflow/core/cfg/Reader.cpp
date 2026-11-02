// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/Reader.hpp"

#include <fstream>
#include <iterator>
#include <string>

#include "ecflow/core/cfg/Configuration.hpp"

namespace ecf::cfg {

CTree Reader::read_file(const std::filesystem::path& file) const {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(file, ec)) {
        throw ConfigurationError("Configuration file '" + file.string() + "' is not a readable regular file");
    }
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        throw ConfigurationError("Configuration file '" + file.string() + "' cannot be read");
    }
    const std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    return read(text, file.string());
}

} // namespace ecf::cfg
