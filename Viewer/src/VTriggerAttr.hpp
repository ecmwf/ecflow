//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VTRIGGERATTR_HPP
#define VTRIGGERATTR_HPP

#include "VAttribute.hpp"

#include <QStringList>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class Expression;

class VTriggerAttr : public VAttribute
{

public:
    VTriggerAttr(VNode *parent,Expression*, int index);

    VAttributeType* type() const;
    QStringList data() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
    static void expressions(const VNode* vnode,std::string& trigger, std::string& complete);
};

#endif // VTRIGGERATTR_HPP

