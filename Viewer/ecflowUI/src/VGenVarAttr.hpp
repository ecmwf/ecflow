//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VGENVARATTR_HPP
#define VGENVARATTR_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;
class Variable;

class VGenVarAttrType : public VAttributeType
{
public:
    explicit VGenVarAttrType();
    QString toolTip(QStringList d) const;
    void encode(const Variable&,QStringList&) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
};

class VGenVarAttr : public VAttribute
{
public:
    VGenVarAttr(VNode *parent,const Variable&,int index);

    VAttributeType* type() const;
    QStringList data(bool firstLine) const;
    std::string strName() const;
    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
    static bool isReadOnly(const std::string&);
};

#endif // VGENVARATTR_HPP

