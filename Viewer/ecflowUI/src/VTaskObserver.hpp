// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VTask.hpp"

class VTaskObserver {
public:
    virtual ~VTaskObserver()            = default;
    virtual void taskChanged(VTask_ptr) = 0;
};
