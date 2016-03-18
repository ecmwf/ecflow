//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VATTRIBUTE_HPP
#define VATTRIBUTE_HPP

#include "VItem.hpp"

#include <QStringList>

class VAttributeType;
class VNode;

class VAttribute : public VItem
{
public:
    VAttribute(VNode* parent,int index);

    VAttribute* isAttribute() const {return const_cast<VAttribute*>(this);}

protected:
    VAttributeType* type_;
    QStringList data_;
    int index_;
};


#endif // VATTRIBUTE_HPP

