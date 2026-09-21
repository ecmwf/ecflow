// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

class SuiteFilter;

class SuiteFilterObserver {
public:
    SuiteFilterObserver()          = default;
    virtual ~SuiteFilterObserver() = default;

    virtual void notifyChange(SuiteFilter*) = 0;
    virtual void notifyDelete(SuiteFilter*) = 0;
};
