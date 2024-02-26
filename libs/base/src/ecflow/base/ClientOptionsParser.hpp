/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
