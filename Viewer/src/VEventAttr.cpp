//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VEventAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VEventAttrType
//================================

VEventAttrType::VEventAttrType() : VAttributeType("event")
{
    dataCount_=3;
    searchKeyToData_["event_name"]=NameIndex;
    searchKeyToData_["event_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
    scanProc_=VEventAttr::scan;
}

QString VEventAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Event<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Status:</b> ";
        t+=(d[ValueIndex] == "1")?"set (true)":"clear (false)";

    }
    return t;
}

QString VEventAttrType::definition(QStringList d) const
{
    QString t="event";
    if(d.count() == dataCount_)
    {
        t+=" " + d[NameIndex];
    }
    return t;
}

void VEventAttrType::encode(const Event& e,QStringList& data) const
{
    data << qName_ <<
              QString::fromStdString(e.name_or_number()) <<
              QString::number((e.value()==true)?1:0);
}

//=====================================================
//
// VEventAttr
//
//=====================================================

VEventAttr::VEventAttr(VNode *parent,const Event& e, int index) : VAttribute(parent,index)
{
    //name_=e.name_or_number();
}

VAttributeType* VEventAttr::type() const
{
    static VAttributeType* atype=VAttributeType::find("event");
    return atype;
}

QStringList VEventAttr::data() const
{
    static VEventAttrType* atype=static_cast<VEventAttrType*>(type());
    QStringList s;
    if(node_ptr node=parent_->node_)
    {
        const std::vector<Event>& v=parent_->node_->events();
        atype->encode(v[index_],s);
    }
    return s;
}

std::string VEventAttr::strName() const
{
    if(parent_->node_)
    {
        const std::vector<Event>& v=parent_->node_->events();
        return v[index_].name_or_number();
    }
    return std::string();
}

void VEventAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(vnode->node_)
    {
        const std::vector<Event>& v=vnode->node_->events();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VEventAttr(vnode,v[i],i));
        }
    }
}
