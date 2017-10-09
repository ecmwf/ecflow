//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VLimiterAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "InLimit.hpp"

//================================
// VLimiterAttrType
//================================

VLimiterAttrType::VLimiterAttrType() : VAttributeType("limiter")
{
    dataCount_=3;
    searchKeyToData_["limiter_name"]=NameIndex;
    searchKeyToData_["limiter_path"]=PathIndex;
    searchKeyToData_["name"]=NameIndex;
    scanProc_=VLimiterAttr::scan;
}

QString VLimiterAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Limiter<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Limit:</b> " + d[NameIndex] + "<br>";
        t+="<b>Node:</b> " + d[PathIndex];

    }
    return t;
}

QString VLimiterAttrType::definition(QStringList d) const
{
    QString t="inlimit";
    if(d.count() == dataCount_)
    {
        t+=" " + d[NameIndex];
    }
    return t;
}

void VLimiterAttrType::encode(const InLimit& lim,QStringList& data) const
{
    data << qName_ <<
           QString::fromStdString(lim.name()) <<
           QString::fromStdString(lim.pathToNode());
}

//=====================================================
//
// VLimiterAttr
//
//=====================================================

VLimiterAttr::VLimiterAttr(VNode *parent,const InLimit& lim, int index) : VAttribute(parent,index)
{
    //name_=lim.name();
}

VAttributeType* VLimiterAttr::type() const
{
    static VAttributeType* atype=VAttributeType::find("limiter");
    return atype;
}

QStringList VLimiterAttr::data(bool /*firstLine*/) const
{
    static VLimiterAttrType* atype=static_cast<VLimiterAttrType*>(type());
    QStringList s;
    if(parent_->node_)
    {
        const std::vector<InLimit>& v=parent_->node_->inlimits();
        atype->encode(v[index_],s);
    }
    return s;
}

std::string VLimiterAttr::strName() const
{
    if(parent_->node_)
    {
        const std::vector<InLimit>& v=parent_->node_->inlimits();
        return v[index_].name();
    }
    return std::string();
}

void VLimiterAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(vnode->node_)
    {
        const std::vector<InLimit>& v=vnode->node_->inlimits();
        int n=static_cast<int>(v.size());
        for(int i=0; i < n; i++)
        {
            vec.push_back(new VLimiterAttr(vnode,v[i],i));
        }
    }
}
