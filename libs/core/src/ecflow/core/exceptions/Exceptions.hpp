/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_exceptions_Exceptions_HPP
#define ecflow_core_exceptions_Exceptions_HPP

#include <stdexcept>

#include "ecflow/core/Message.hpp"

namespace ecf {

struct Exception : public std::runtime_error
{
    explicit Exception(const char* what)
        : std::runtime_error(what) {}
    explicit Exception(const std::string& what)
        : std::runtime_error(what) {}
};

struct InvalidArgument : public Exception
{
    explicit InvalidArgument(const char* what)
        : Exception(what) {}
    explicit InvalidArgument(const std::string& what)
        : Exception(what) {}
};

} // namespace ecf

#define THROW_EXCEPTION(exception, message) \
    do {                                    \
        throw exception(MESSAGE(message));  \
    } while (0)

#define THROW_RUNTIME(message) THROW_EXCEPTION(std::runtime_error, message)

#endif /* ecflow_core_exceptions_Exceptions_HPP */
