//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VLABEL_HPP
#define VLABEL_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class Label;

class VLabelAttrType : public VAttributeType
{
public:
    explicit VLabelAttrType();
    QString toolTip(QStringList d) const;
    QString definition(QStringList d) const;
    void encode(const Label& label,QStringList& data,bool firstLine) const;
    void encode_empty(QStringList& data) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
};

class VLabelAttr : public VAttribute
{
public:
    VLabelAttr(VNode *parent,const Label&,int index);

    int lineNum() const;
    VAttributeType* type() const;
    QStringList data(bool firstLine) const;
    std::string strName() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VLABEL_HPP
