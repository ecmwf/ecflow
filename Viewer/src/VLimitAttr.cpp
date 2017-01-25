//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VLimitAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VLimitAttrType
//================================

class VLimitAttrType : public VAttributeType
{
public:
    explicit VLimitAttrType();
    QString toolTip(QStringList d) const;
    void encode(limit_ptr,QStringList&) const;

private:
     enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MaxIndex=3};
};


VLimitAttrType::VLimitAttrType() : VAttributeType("limit")
{
    dataCount_=4;
    searchKeyToData_["limit_name"]=NameIndex;
    searchKeyToData_["limit_value"]=ValueIndex;
    searchKeyToData_["limit_max"]=MaxIndex;
    searchKeyToData_["name"]=NameIndex;
}

QString VLimitAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Limit<br>";
    if(d.count()  == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Value:</b> " + d[ValueIndex] + "<br>";
        t+="<b>Maximum:</b> " + d[MaxIndex];
    }
    return t;
}

void VLimitAttrType::encode(limit_ptr lim,QStringList& data) const
{
    data << qName_ <<
        QString::fromStdString(lim->name()) <<
        QString::number(lim->value()) <<
        QString::number(lim->theLimit());
}

static VLimitAttrType atype;

//=====================================================
//
// VLimitAttr
//
//=====================================================

VLimitAttr::VLimitAttr(VNode *parent,limit_ptr lim, int index) : VAttribute(parent,index)
{
    name_=lim->name();
}

VAttributeType* VLimitAttr::type() const
{
    return &atype;
}

QStringList VLimitAttr::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<limit_ptr>& v=node->limits();
        atype.encode(v[index_],s);
    }
    return s;
}

void VLimitAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<limit_ptr>& v=node->limits();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VLimitAttr(vnode,v[i],i));
        }
    }
}
