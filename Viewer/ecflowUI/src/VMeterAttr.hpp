//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VMETERATTR_HPP
#define VMETERATTR_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class Meter;

class VMeterAttrType : public VAttributeType
{
public:
    explicit VMeterAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const Meter&,QStringList&) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MinIndex=3, MaxIndex=4,ThresholdIndex=5};
};

class VMeterAttr : public VAttribute
{

public:
    VMeterAttr(VNode *parent,const Meter&,int index);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VMETER_HPP
