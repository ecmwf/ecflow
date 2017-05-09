//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VLIMITERATTR_HPP
#define VLIMITERATTR_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class InLimit;

class VLimiterAttrType : public VAttributeType
{
public:
    explicit VLimiterAttrType();
    QString toolTip(QStringList d) const;
    QString definition(QStringList d) const;
    void encode(const InLimit&,QStringList&) const;
    void scan(VNode* vnode,std::vector<VAttribute*>& vec);
    int totalNum(VNode* vnode);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,PathIndex=2};
};

class VLimiterAttr : public VAttribute
{

public:
    VLimiterAttr(VNode *parent,const InLimit&,int index);

    VAttributeType* type() const;
    QStringList data() const;
    std::string strName() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VLIMITERATTR_HPP

