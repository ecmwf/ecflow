//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VDATE_HPP
#define VDATE_HPP

#include "VAttribute.hpp"

#include <QStringList>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class DateAttr;
class DayAttr;

class VDateAttr : public VAttribute
{

public:
    enum DataType {DateData,DayData};

    VDateAttr(VNode *parent,const DateAttr&,int index);
    VDateAttr(VNode *parent,const DayAttr&,int index);

    VAttributeType* type() const;
    QStringList data() const;
    std::string strName() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);

protected:
    DataType dataType_;
};

#endif // VDATE_HPP
