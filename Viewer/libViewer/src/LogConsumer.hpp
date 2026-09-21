// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

class LogConsumer {
public:
    virtual ~LogConsumer()                      = default;
    virtual void addLogLine(const std::string&) = 0;
};
