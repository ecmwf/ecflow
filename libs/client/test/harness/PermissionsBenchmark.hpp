// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ecflow/core/Filesystem.hpp"

///
/// @brief Helpers of the benchmark of the Permissions attributes (p_permissions).
///
namespace ecf::test::benchmark {

///
/// @brief Parses a positive integer, refusing anything else (sign, white space, trailing characters, overflow).
///
/// @param[in] name The name of the parameter, for the diagnostic
/// @param[in] text The text
/// @return The value.
/// @throws std::invalid_argument when the text is not a positive integer.
///
inline int parse_positive(const std::string& name, const std::string& text) {
    if (text.empty() || text.size() > 9 ||
        !std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        throw std::invalid_argument(name + ": expected a positive integer, not '" + text + "'");
    }
    const int value = std::stoi(text);
    if (value <= 0) {
        throw std::invalid_argument(name + ": expected a positive integer, not '" + text + "'");
    }
    return value;
}

///
/// @brief Returns a positive integer parameter from the environment, or its default when unset.
///
/// @param[in] name The environment variable
/// @param[in] fallback The default
/// @return The value.
/// @throws std::invalid_argument when the variable is set to anything but a positive integer.
///
inline int parameter(const char* name, int fallback) {
    const char* value = std::getenv(name);
    return value ? parse_positive(name, value) : fallback;
}

///
/// @brief Holds the median and the maximum of a sample.
///
struct Summary
{
    double median{0}; ///< The median (the upper one for an even count)
    double max{0};    ///< The maximum
};

///
/// @brief Summarises a sample.
///
/// @param[in] values The sample
/// @return Its median and maximum.
/// @throws std::invalid_argument when the sample is empty.
///
inline Summary summarise(std::vector<double> values) {
    if (values.empty()) {
        throw std::invalid_argument("summarise: empty sample");
    }
    std::sort(values.begin(), values.end());
    return Summary{values[values.size() / 2], values.back()};
}

///
/// @brief Parses a CPU time as printed by ps: `[[dd-]hh:]mm:ss[.cc]`.
///
/// @param[in] text The text
/// @return The time, in seconds.
/// @throws std::invalid_argument when the text is not such a time (empty included).
///
inline double parse_cpu_time(const std::string& text) {
    // Days require hours; minutes and seconds below 60; a fraction of a second only
    static const std::regex grammar(R"(^(?:(\d+)-(\d+):|(\d+):)?(\d+):(\d+(?:\.\d+)?)$)");
    std::smatch match;
    if (!std::regex_match(text, match, grammar)) {
        throw std::invalid_argument("not a CPU time: '" + text + "'");
    }
    const double days    = match[1].matched ? std::stod(match[1]) : 0;
    const double hours   = match[2].matched ? std::stod(match[2]) : (match[3].matched ? std::stod(match[3]) : 0);
    const double minutes = std::stod(match[4]);
    const double seconds = std::stod(match[5]);
    if ((match[1].matched && hours >= 24) || minutes >= 60 || seconds >= 60) {
        throw std::invalid_argument("not a CPU time: '" + text + "'");
    }
    return ((days * 24 + hours) * 60 + minutes) * 60 + seconds;
}

///
/// @brief Parses a resident memory size, in KiB, as printed by ps.
///
/// @param[in] text The text
/// @return The size, in MiB.
/// @throws std::invalid_argument when the text is not a positive integer.
///
inline double parse_memory_kib(const std::string& text) {
    return static_cast<double>(parse_positive("rss", text)) / 1024.0;
}

///
/// @brief Runs a shell command and returns its standard output, without leading and trailing white space.
///
/// @param[in] command The command
/// @return The output.
/// @throws std::runtime_error when the command cannot be run, or fails.
///
inline std::string run(const std::string& command) {
    std::array<char, 256> buffer{};
    std::string output;
    FILE* pipe = ::popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("cannot run: " + command);
    }
    while (std::fgets(buffer.data(), buffer.size(), pipe)) {
        output += buffer.data();
    }
    if (::pclose(pipe) != 0) {
        throw std::runtime_error("failed: " + command);
    }
    output.erase(output.find_last_not_of(" \n\r\t") + 1);
    output.erase(0, output.find_first_not_of(" \n\r\t"));
    return output;
}

///
/// @brief Writes a report, checking that every step succeeds.
///
/// @param[in] path The destination
/// @param[in] text The content
/// @throws std::runtime_error naming the destination when the file cannot be opened, written or closed.
///
inline void write_report(const fs::path& path, const std::string& text) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open the report " + path.string());
    }
    file << text;
    file.close();
    if (!file) {
        throw std::runtime_error("cannot write the report " + path.string());
    }
}

} // namespace ecf::test::benchmark
