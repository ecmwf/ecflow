/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_ClientOptionsParser_HPP
#define ecflow_base_ClientOptionsParser_HPP

#include <string>
#include <vector>

#include <boost/program_options.hpp>

namespace ecf {

struct ClientOptionsParser
{
    using option_t        = boost::program_options::option;
    using option_set_t    = std::vector<option_t>;
    using arguments_set_t = std::vector<std::string>;

    option_set_t operator()(arguments_set_t& args);
};

} // namespace ecf

#endif /* ecflow_base_ClientOptionsParser_HPP */
