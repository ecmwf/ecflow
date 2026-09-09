/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VTaskObserver_HPP
#define ecflow_viewer_VTaskObserver_HPP

#include "VTask.hpp"

class VTaskObserver {
public:
    virtual ~VTaskObserver()            = default;
    virtual void taskChanged(VTask_ptr) = 0;
};

#endif /* ecflow_viewer_VTaskObserver_HPP */
