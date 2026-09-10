/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_client_Help_HPP
#define ecflow_client_Help_HPP

#include <memory>

#include <boost/program_options.hpp>

class Help {
public:
    using description_t  = boost::program_options::options_description;
    using descriptions_t = std::vector<boost::shared_ptr<boost::program_options::option_description>>;

    Help(const description_t& description, const std::string& topic);
    ~Help();

    friend std::ostream& operator<<(std::ostream& os, const Help& help);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif /* ecflow_client_Help_HPP */
