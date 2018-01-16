//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef PROPERTYMAPPER_INC_
#define PROPERTYMAPPER_INC_

#include "VProperty.hpp"

class PropertyMapper
{
public:
    PropertyMapper(const std::vector<std::string>&,VPropertyObserver* obs);
    ~PropertyMapper();
    VProperty* find(const std::string& path,bool failOnError=false) const;
    void initObserver(VPropertyObserver *obs) const;

private:
    VPropertyObserver* obs_;
    std::vector<VProperty*> props_;
};

#endif

