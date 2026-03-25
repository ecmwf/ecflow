/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Filesystem.hpp"

#include <array>
#include <random>

namespace ecf {
namespace fsx {

std::filesystem::path unique_path(std::string model) {
    static const std::array hex_chars = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    // Setup randomness
    std::random_device rdev;
    std::mt19937 rgen(rdev());
    std::uniform_int_distribution idist(0, 15);

    for (char& current : model) {
        if (current == '%') {
            // Generate random value (between [0, 15])
            auto r = idist(rgen);
            // Get the corresponding hex character
            auto c = hex_chars[r];
            // Replace the '%' with the random character
            current = c;
        }
    }
    return {model};
}

std::chrono::system_clock::time_point last_write_time(const std::filesystem::path& path) {
    // Get the last write time...
    auto f_time = std::filesystem::last_write_time(path);

    using s_clock = std::chrono::system_clock;
    using f_clock = decltype(f_time)::clock;

    // Establish the difference between system_clock and 'file_clock'
    auto s_now      = s_clock::now().time_since_epoch();
    auto f_now      = f_clock::now().time_since_epoch();
    auto difference = s_now - f_now;

    // Convert to system_clock time_point
    auto f_since_epoch = f_time.time_since_epoch();
    auto s_since_epoch = f_since_epoch + difference;
    auto s_time        = s_clock::time_point{std::chrono::duration_cast<std::chrono::seconds>(s_since_epoch)};

    return s_time;
}

} // namespace fsx
} // namespace ecf
