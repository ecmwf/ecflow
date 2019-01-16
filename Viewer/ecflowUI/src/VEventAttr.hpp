//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VEVENT_HPP
#define VEVENT_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class Event;

class VEventAttrType : public VAttributeType
{
public:
    explicit VEventAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const Event&,QStringList&) const;

private:
     enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
};

class VEventAttr : public VAttribute
{
public:
    VEventAttr(VNode *parent,const Event&,int index);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VEVENT_HPP
