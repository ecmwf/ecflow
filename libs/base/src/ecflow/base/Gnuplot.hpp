/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Gnuplot_HPP
#define ecflow_base_Gnuplot_HPP

#include <string>
#include <vector>

#include "ecflow/core/Host.hpp"

///
/// \brief Encompasses functionality for plotting server load
///

namespace ecf {

class Gnuplot {
public:
    Gnuplot()               = delete;
    Gnuplot(const Gnuplot&) = delete;
    Gnuplot(const std::string& log_file,
            const std::string& host,
            const std::string& port,
            size_t no_of_suites_to_plot = 5);

    Gnuplot& operator=(const Gnuplot&) = delete;

    /// parse the log file and show gnuplot of server load
    /// Include the suite most contributing to the load
    /// generates two files
    ///    o <host>.<port>.gnuplot.dat
    ///    o <host>.<port>.gnuplot.script
    void show_server_load() const;

private:
    std::string log_file_;
    Host host_;
    std::string port_;
    size_t no_of_suites_to_plot_;

private:
    struct SuiteLoad
    {
        explicit SuiteLoad(const std::string& name)
            : suite_name_(name),
              request_per_second_(1),
              total_request_per_second_(1) {}

        std::string suite_name_;
        size_t request_per_second_;
        size_t total_request_per_second_;
    };

    /// Returns that path to file created by this function.
    /// The file is used by gnuplot to show the server load.
    ///  Can throw exceptions
    std::string create_gnuplot_file(std::vector<SuiteLoad>& suite_vec, const std::string& input_data) const;

    /// returns the path to the gnuplot script
    std::string create_gnuplot_script(const std::string& path_to_file,
                                      const std::vector<SuiteLoad>& suite_vec,
                                      size_t no_of_suites_to_plot,
                                      const std::string& script) const;

    static bool extract_suite_path(const std::string& line,
                                   bool child_cmd,
                                   std::vector<SuiteLoad>& suite_vec,
                                   size_t& column_index // 0 based
    );
};
} // namespace ecf

#endif /* ecflow_base_Gnuplot_HPP */
