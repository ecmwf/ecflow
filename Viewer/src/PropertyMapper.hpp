// Copyright 2014 ECMWF.

#ifndef PROPERTYMAPPER_INC_
#define PROPERTYMAPPER_INC_

#include "VProperty.hpp"


class PropertyMapper
{
public:
    PropertyMapper(const std::vector<std::string>&,VPropertyObserver* obs);
    ~PropertyMapper();
    VProperty* find(const std::string& path) const;

private:
    VPropertyObserver* obs_;
    std::vector<VProperty*> props_;
};

#endif

