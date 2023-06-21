/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef CLIENT_HELP_HPP
#define CLIENT_HELP_HPP

#include <memory>

#include <boost/program_options.hpp>

class Help {
public:
    using descriptions_t = boost::program_options::options_description;

    Help(const descriptions_t& descriptions, const std::string& topic);
    ~Help();

    friend std::ostream& operator<<(std::ostream& os, const Help& help);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif
