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

class VLimiterAttrType : public VAttributeType
{
public:
    explicit VLimiterAttrType();
    QString toolTip(QStringList d) const;
    void encode(const InLimit&,QStringList&) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,PathIndex=2};
};


VLimiterAttrType::VLimiterAttrType() : VAttributeType("limiter")
{
    dataCount_=3;
    searchKeyToData_["limiter_name"]=NameIndex;
    searchKeyToData_["limiter_path"]=PathIndex;
    searchKeyToData_["name"]=NameIndex;
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

void VLimiterAttrType::encode(const InLimit& lim,QStringList& data) const
{
    data << qName_ <<
           QString::fromStdString(lim.name()) <<
           QString::fromStdString(lim.pathToNode());
}

static VLimiterAttrType atype;

//=====================================================
//
// VLimiterAttr
//
//=====================================================

VLimiterAttr::VLimiterAttr(VNode *parent,const InLimit& lim, int index) : VAttribute(parent,index)
{
    name_=lim.name();
}

VAttributeType* VLimiterAttr::type() const
{
    return &atype;
}

QStringList VLimiterAttr::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<InLimit>& v=node->inlimits();
        atype.encode(v[index_],s);
    }
    return s;
}

void VLimiterAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<InLimit>& v=node->inlimits();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VLimiterAttr(vnode,v[i],i));
        }
    }
}
