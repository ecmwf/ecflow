//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VLIMITATTR_HPP
#define VLIMITATTR_HPP

#include <QStringList>
#include <string>
#include <vector>

#include "LimitFwd.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class VLimitAttrType : public VAttributeType
{
public:
    explicit VLimitAttrType();
    QString toolTip(QStringList d) const;
    QString definition(QStringList d) const;
    void encode(limit_ptr,QStringList&) const;

private:
     enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MaxIndex=3};
};

class VLimitAttr : public VAttribute
{

public:
    VLimitAttr(VNode *parent,limit_ptr,int index);

    VAttributeType* type() const;
    QStringList data(bool firstLine) const;
    std::string strName() const;
    QStringList paths() const;
    void removePaths(const std::vector<std::string>& paths);
    void resetPaths();

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VLIMITATTR_HPP

