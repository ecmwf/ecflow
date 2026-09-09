/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_LogConsumer_HPP
#define ecflow_viewer_LogConsumer_HPP

#include <string>

class LogConsumer {
public:
    virtual ~LogConsumer()                      = default;
    virtual void addLogLine(const std::string&) = 0;
};

#endif /* ecflow_viewer_LogConsumer_HPP */
