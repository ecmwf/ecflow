//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VDateAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "DateAttr.hpp"
#include "DayAttr.hpp"

//================================
// VDateAttrType
//================================

class VDateAttrType : public VAttributeType
{
public:
    explicit VDateAttrType();
    QString toolTip(QStringList d) const;
    void encode(const DateAttr& d,QStringList& data);
    void encode(const DayAttr& d,QStringList& data);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
};


VDateAttrType::VDateAttrType() : VAttributeType("date")
{
    dataCount_=2;
    searchKeyToData_["date_name"]=NameIndex;
    searchKeyToData_["name"]=NameIndex;
}

QString VDateAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Date<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex];
    }
    return t;
}

void VDateAttrType::encode(const DateAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

void VDateAttrType::encode(const DayAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

static VDateAttrType atype;

//=====================================================
//
// VDateAttr
//
//=====================================================

VDateAttr::VDateAttr(VNode *parent,const DateAttr& t, int index) :
    VAttribute(parent,index),
    dataType_(DateData)
{
    name_=t.name();
}

VDateAttr::VDateAttr(VNode *parent,const DayAttr& t, int index) :
    VAttribute(parent,index),
    dataType_(DayData)
{
    name_=t.name();
}

VAttributeType* VDateAttr::type() const
{
    return &atype;
}

QStringList VDateAttr::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        if(dataType_ == DateData)
        {
            const std::vector<DateAttr>& v=node->dates();
            if(index_ < v.size())
                atype.encode(v[index_],s);
        }
        else if(dataType_ == DayData)
        {
            const std::vector<DayAttr>& v=node->days();
            if(index_ < v.size())
                atype.encode(v[index_],s);
        }
    }
    return s;
}

void VDateAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<DateAttr>& dateV=node->dates();
        const std::vector<DayAttr>& dayV=node->days();

        int n=dateV.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VDateAttr(vnode,dateV[i],i));
        }

        n=dayV.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VDateAttr(vnode,dayV[i],i));
        }
    }
}
