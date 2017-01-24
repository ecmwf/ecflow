//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VMETER_HPP
#define VMETER_HPP

#include "VAttribute.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class Meter;

class VMeter : public VAttribute
{

public:
    VMeter(VNode *parent,const Meter&,int index);

    VAttributeType* type() const;
    QStringList data() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VMETER_HPP
