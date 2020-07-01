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

#include "Str.hpp"
#include "PrintStyle.hpp"
#include "DateAttr.hpp"
#include "DayAttr.hpp"

//================================
// VDateAttrType
//================================

VDateAttrType::VDateAttrType() : VAttributeType("date")
{
    dataCount_=3;
    searchKeyToData_["date_name"]=NameIndex;
    searchKeyToData_["name"]=NameIndex;
    scanProc_=VDateAttr::scan;
}

QString VDateAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Date<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] +
        "<br><b>Status: </b>" + ((d[FreeIndex] == "1")?"Free":"Holding");
    }
    return t;
}

QString VDateAttrType::definition(QStringList d) const
{
    QString t;
    if(d.count() == dataCount_)
    {
        t=d[NameIndex];
    }
    return t;
}

void VDateAttrType::encode(const ecf::Calendar& calendar, const DateAttr& d,QStringList& data)
{
    PrintStyle(PrintStyle::MIGRATE);
    std::string s;
    d.print(s);
    ecf::Str::removeTrailingBreakAndSimplify(s);
    data << qName_ << QString::fromStdString(s) <<
        (d.isFree(calendar)?"1":"0");
}

void VDateAttrType::encode(const ecf::Calendar& calendar, const DayAttr& d,QStringList& data)
{
    PrintStyle(PrintStyle::MIGRATE);
    std::string s;
    d.print(s);
    ecf::Str::removeTrailingBreakAndSimplify(s);
    data << qName_ << QString::fromStdString(s)<<
        (d.isFree(calendar)?"1":"0");
}

//=====================================================
//
// VDateAttr
//
//=====================================================

VDateAttr::VDateAttr(VNode *parent,const DateAttr& t, int index) :
    VAttribute(parent,index),
    dataType_(DateData)
{
    //name_=t.name();
}

VDateAttr::VDateAttr(VNode *parent,const DayAttr& t, int index) :
    VAttribute(parent,index),
    dataType_(DayData)
{
    //name_=t.name();
}

VAttributeType* VDateAttr::type() const
{
    static VAttributeType* atype=VAttributeType::find("date");
    return atype;
}

QStringList VDateAttr::data(bool /*firstLine*/) const
{
    static auto* atype=static_cast<VDateAttrType*>(type());
    QStringList s;
    if(parent_->node_)
    {
        const ecf::Calendar& calendar = parent_->calendar();
        if(dataType_ == DateData)
        {
            const std::vector<DateAttr>& v=parent_->node_->dates();
            if(index_ < static_cast<int>(v.size()))
                atype->encode(calendar, v[index_],s);
        }
        else if(dataType_ == DayData)
        {
            const std::vector<DayAttr>& v=parent_->node_->days();
            if(index_ < static_cast<int>(v.size()))
                atype->encode(calendar, v[index_],s);
        }
    }
    return s;
}

std::string VDateAttr::strName() const
{
    if(parent_->node_)
    {
        PrintStyle(PrintStyle::MIGRATE);

        if(dataType_ == DateData)
        {
            const std::vector<DateAttr>& v=parent_->node_->dates();
            if(index_ < static_cast<int>(v.size())) {
                std::string s;
                v[index_].print(s);
                ecf::Str::removeTrailingBreakAndSimplify(s);
                return s;
            }
        }
        else if(dataType_ == DayData)
        {
            const std::vector<DayAttr>& v=parent_->node_->days();
            if(index_ < static_cast<int>(v.size())) {
                std::string s;
                v[index_].print(s);
                ecf::Str::removeTrailingBreakAndSimplify(s);
                return s;
            }
        }
    }
    return std::string();
}

void VDateAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(vnode->node_)
    {
        const std::vector<DateAttr>& dateV=vnode->node_->dates();
        const std::vector<DayAttr>& dayV=vnode->node_->days();

        auto n=static_cast<int>(dateV.size());
        for(int i=0; i < n; i++)
        {
            vec.push_back(new VDateAttr(vnode,dateV[i],i));
        }

        n=static_cast<int>(dayV.size());
        for(int i=0; i < n; i++)
        {
            vec.push_back(new VDateAttr(vnode,dayV[i],i));
        }
    }
}
