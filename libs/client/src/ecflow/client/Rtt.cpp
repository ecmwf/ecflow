/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/client/Rtt.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

#include "ecflow/core/Chrono.hpp"
#include "ecflow/core/File.hpp"

namespace ecf {

Rtt* Rtt::instance_ = nullptr;

void Rtt::create(const std::string& filename) {
    if (instance_ == nullptr) {
        instance_ = new Rtt(filename);
    }
}

void Rtt::destroy() {
    delete instance_;
    instance_ = nullptr;
}

Rtt::Rtt(const std::string& filename) : file_(filename.c_str(), std::ios::out | std::ios::app) {
    if (!file_.is_open()) {
        std::cerr << "Rtt::Rtt Could not open file '" << filename << "'\n";
        throw std::runtime_error("Rtt::Rtt: Could not open file " + filename);
    }
}

Rtt::~Rtt() = default;

void Rtt::log(const std::string& message) {
    file_ << message << std::endl;
}

void rtt(const std::string& message) {
    if (Rtt::instance()) {
        Rtt::instance()->log(message);
    }
}

std::string Rtt::analysis(const std::string& filename) {
    std::vector<std::string> lines;
    if (!File::splitFileIntoLines(filename, lines)) {
        std::cout << "Rtt::analysis: could not open file " << filename << " (" << strerror(errno) << ")\n";
        return std::string();
    }

    // Typical format
    // localhost:3141 --ping :ma0 rtt:00:00:00.001082
    // localhost:3141 --log=new Test/data/ECF_HOME/test_wait_cmd/test_wait_cmd.def_log  :ma0 rtt:00:00:00.000930
    // localhost:3141 --zombie_get :ma0 rtt:00:00:00.003982

    /// Extract the command name and time, and add to map, to compute averages, min,max & standard deviation
    std::map<std::string, std::vector<boost::posix_time::time_duration>> cmd_time_map;
    size_t max_cmd_size = 0;
    for (auto& line : lines) {

        if (line.empty()) {
            continue;
        }
        //      cout << i << ":" << lines[i] << "   ";
        std::string::size_type dash    = line.find("--");
        std::string::size_type rtt_pos = line.find(Rtt::tag());
        if (dash == std::string::npos) {
            continue;
        }
        if (rtt_pos == std::string::npos) {
            continue;
        }

        int cmd_length                = 0;
        std::string::size_type equals = line.find("=", dash);
        std::string::size_type space  = line.find(" ", dash);
        if (equals != std::string::npos) {
            cmd_length = equals;
        }
        else if (space != std::string::npos) {
            cmd_length = space;
        }
        std::string cmd = line.substr(0, cmd_length);
        max_cmd_size    = std::max(max_cmd_size, cmd.size());

        std::string time = line.substr(rtt_pos + 4);
        auto td          = boost::posix_time::duration_from_string(time);

        auto cmd_iterator = cmd_time_map.find(cmd);
        if (cmd_iterator == cmd_time_map.end()) {
            std::vector<boost::posix_time::time_duration> vec;
            vec.push_back(td);
            std::pair<std::string, std::vector<boost::posix_time::time_duration>> p = std::make_pair(cmd, vec);
            cmd_time_map.insert(p);
        }
        else {
            (*cmd_iterator).second.push_back(td);
        }
    }

    auto total         = boost::posix_time::time_duration(0, 0, 0, 0);
    int total_requests = 0;

    // Create title
    std::stringstream ss;
    ss << std::left << std::setw(max_cmd_size + 1) << "Command" << std::right << std::setw(5) << "count" << std::setw(9)
       << "min" << std::setw(9) << "average" << std::setw(9) << "max" << std::setw(9) << std::right << "std\n";
    for (std::pair<std::string, std::vector<boost::posix_time::time_duration>> p : cmd_time_map) {

        auto average_td = boost::posix_time::time_duration(0, 0, 0, 0);
        auto min        = boost::posix_time::time_duration(24, 59, 59, 0);
        auto max        = boost::posix_time::time_duration(0, 0, 0, 0);
        for (const auto& i : p.second) {
            average_td += i;
            total_requests++;
            total += i;
            min = std::min(min, i);
            max = std::max(max, i);
        }

        ss << std::left << std::setw(max_cmd_size + 1) << p.first << std::setw(5) << std::right << p.second.size();
        if (p.second.empty()) {
            ss << std::setw(9) << std::right << p.second[0].total_microseconds() << " ? ";
        }
        else if (p.second.size() == 1) {
            ss << std::setw(9) << std::right << p.second[0].total_microseconds();
        }
        else {
            int average = average_td.total_microseconds() / p.second.size();

            //          bool debug = false;
            //          if (p.first == "begin")   debug = true;

            // compute standard deviation
            unsigned int total_diff_from_avg = 0;
            for (auto& i : p.second) {
                int diff         = i.total_microseconds() - average;
                int diff_squared = diff * diff;
                total_diff_from_avg += diff_squared;
                //             if (debug) cout << "diff: " << diff << " diff_squared: " << diff_squared << "
                //             total_diff_from_avg: " << total_diff_from_avg << "\n";
            }

            double avg = total_diff_from_avg / p.second.size();
            auto stdd  = (int)sqrt(avg);
            //          if (debug) cout << "avg: " << avg << " stdd: " << stdd << "\n";

            ss << std::setw(9) << std::right << min.total_microseconds();
            ss << std::setw(9) << std::right << average;
            ss << std::setw(9) << std::right << max.total_microseconds();
            ss << std::setw(9) << std::right << stdd;
        }
        ss << "\n";
    }
    ss << "\ntotal round trip time " << to_simple_string(total) << " for " << total_requests << " requests\n";
    return ss.str();
}

} // namespace ecf
