/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VariableModelDataObserver_HPP
#define ecflow_viewer_VariableModelDataObserver_HPP

class VariableModelDataHandler;

class VariableModelDataObserver {
public:
    VariableModelDataObserver()                           = default;
    virtual void notifyCleared(VariableModelDataHandler*) = 0;
    virtual void notifyUpdated(VariableModelDataHandler*) = 0;
};

#endif /* ecflow_viewer_VariableModelDataObserver_HPP */
