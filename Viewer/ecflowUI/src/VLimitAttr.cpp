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

VLimitAttrType::VLimitAttrType() : VAttributeType("limit")
{
    dataCount_=4;
    searchKeyToData_["limit_name"]=NameIndex;
    searchKeyToData_["limit_value"]=ValueIndex;
    searchKeyToData_["limit_max"]=MaxIndex;
    searchKeyToData_["name"]=NameIndex;
    scanProc_=VLimitAttr::scan;
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

QString VLimitAttrType::definition(QStringList d) const
{
    QString t="limit";
    if(d.count() == dataCount_)
    {
        t+=" " + d[NameIndex] + " " + d[MaxIndex];
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

//=====================================================
//
// VLimitAttr
//
//=====================================================

VLimitAttr::VLimitAttr(VNode *parent,limit_ptr lim, int index) : VAttribute(parent,index)
{
    //name_=lim->name();
}

VAttributeType* VLimitAttr::type() const
{
    static VAttributeType* atype=VAttributeType::find("limit");
    return atype;
}

QStringList VLimitAttr::data(bool /*firstLine*/) const
{
    static VLimitAttrType* atype=static_cast<VLimitAttrType*>(type());
    QStringList s;
    if(parent_->node_)
    {
        const std::vector<limit_ptr>& v=parent_->node_->limits();
        atype->encode(v[index_],s);
    }
    return s;
}

std::string VLimitAttr::strName() const
{
    if(parent_->node_)
    {
        const std::vector<limit_ptr>& v=parent_->node_->limits();
        return v[index_]->name();
    }
    return std::string();
}

QStringList VLimitAttr::paths() const
{
    QStringList lst;
    if(parent_->node_)
    {
        if(index_ >=0 && index_ < static_cast<int>(parent_->node_->limits().size()))
        {
            const std::set<std::string>& s=parent_->node_->limits()[index_]->paths();
            for(const auto & it : s)
            {
                lst << QString::fromStdString(it);
            }
        }
    }
    return lst;
}

void VLimitAttr::removePaths(const std::vector<std::string>& paths)
{
    if(parent_->node_)
    {
        if(index_ >=0 && index_ < static_cast<int>(parent_->node_->limits().size()))
        {
            limit_ptr lim=parent_->node_->limits()[index_];
            for(const auto & path : paths)
            {
                lim->delete_path(path);
            }
            return;
         }
    }
}

void VLimitAttr::resetPaths()
{
    if(parent_->node_)
    {
        if(index_ >=0 && index_ < static_cast<int>(parent_->node_->limits().size()))
        {
            parent_->node_->limits()[index_]->reset();
            return;
         }
    }
}

void VLimitAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(vnode->node_)
    {
        const std::vector<limit_ptr>& v=vnode->node_->limits();
        int n=static_cast<int>(v.size());
        for(int i=0; i < n; i++)
        {
            vec.push_back(new VLimitAttr(vnode,v[i],i));
        }
    }
}

