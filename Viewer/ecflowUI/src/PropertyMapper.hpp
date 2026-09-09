/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_PropertyMapper_HPP
#define ecflow_viewer_PropertyMapper_HPP

#include "VProperty.hpp"

class PropertyMapper {
public:
    PropertyMapper(const std::vector<std::string>&, VPropertyObserver* obs);
    ~PropertyMapper();
    VProperty* find(const std::string& path, bool failOnError = false) const;
    void initObserver(VPropertyObserver* obs) const;

private:
    VPropertyObserver* obs_;
    std::vector<VProperty*> props_;
};

#endif /* ecflow_viewer_PropertyMapper_HPP */
