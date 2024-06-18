/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_exceptions_Exceptions_HPP
#define ecflow_core_exceptions_Exceptions_HPP

#include <stdexcept>

namespace ecf {

struct Exception : public std::runtime_error
{
    explicit Exception(const char* what) : std::runtime_error(what) {}
    explicit Exception(const std::string& what) : std::runtime_error(what) {}
};

struct InvalidArgument : public Exception
{
    explicit InvalidArgument(const char* what) : Exception(what) {}
    explicit InvalidArgument(const std::string& what) : Exception(what) {}
};

} // namespace ecf

#endif // ecflow_core_exceptions_Exceptions_HPP
