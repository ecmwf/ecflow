// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

class VariableModelDataHandler;

class VariableModelDataObserver {
public:
    VariableModelDataObserver()                           = default;
    virtual void notifyCleared(VariableModelDataHandler*) = 0;
    virtual void notifyUpdated(VariableModelDataHandler*) = 0;
};
