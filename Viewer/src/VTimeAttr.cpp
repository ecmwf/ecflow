//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VTimeAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

//================================
// VTimeAttrType
//================================

class VTimeAttrType : public VAttributeType
{
public:
    explicit VTimeAttrType();
    QString toolTip(QStringList d) const;
    void encode(const ecf::TimeAttr& d,QStringList& data);
    void encode(const ecf::TodayAttr& d,QStringList& data);
    void encode(const ecf::CronAttr& d,QStringList& data);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
};


VTimeAttrType::VTimeAttrType() : VAttributeType("time")
{
    dataCount_=2;
    searchKeyToData_["time_name"]=NameIndex;
    searchKeyToData_["name"]=NameIndex;
}

QString VTimeAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Time<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex];
    }
    return t;
}

void VTimeAttrType::encode(const ecf::TimeAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

void VTimeAttrType::encode(const ecf::TodayAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

void VTimeAttrType::encode(const ecf::CronAttr& d,QStringList& data)
{
    data << qName_ << QString::fromStdString(d.name());
}

static VTimeAttrType atype;

//=====================================================
//
// VTimeAttr
//
//=====================================================

VTimeAttr::VTimeAttr(VNode *parent,const ecf::TimeAttr& t, int index) :
    VAttribute(parent,index),
    dataType_(TimeData)
{
    name_=t.name();
}

VTimeAttr::VTimeAttr(VNode *parent,const ecf::TodayAttr& t, int index) :
    VAttribute(parent,index),
    dataType_(TodayData)
{
    name_=t.name();
}

VTimeAttr::VTimeAttr(VNode *parent,const ecf::CronAttr& t, int index) :
    VAttribute(parent,index),
    dataType_(CronData)
{
    name_=t.name();
}

VAttributeType* VTimeAttr::type() const
{
    return &atype;
}

QStringList VTimeAttr::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        if(dataType_ == TimeData)
        {
            const std::vector<ecf::TimeAttr>& v=node->timeVec();
            if(index_ < v.size())
                atype.encode(v[index_],s);
        }
        else if(dataType_ == TodayData)
        {
            const std::vector<ecf::TodayAttr>& v=node->todayVec();
            if(index_ < v.size())
                atype.encode(v[index_],s);
        }
        else if(dataType_ == CronData)
        {
            const std::vector<ecf::CronAttr>& v=node->crons();
            if(index_ < v.size())
                atype.encode(v[index_],s);
        }
    }
    return s;
}

void VTimeAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<ecf::TimeAttr>& tV=node->timeVec();
        const std::vector<ecf::TodayAttr>& tdV=node->todayVec();
        const std::vector<ecf::CronAttr>& cV=node->crons();

        int n=tV.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VTimeAttr(vnode,tV[i],i));
        }

        n=tdV.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VTimeAttr(vnode,tdV[i],i));
        }

        n=cV.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VTimeAttr(vnode,cV[i],i));
        }
    }
}
