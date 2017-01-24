//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VEvent.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VEventType
//================================

class VEventType : public VAttributeType
{
public:
    explicit VEventType();
    QString toolTip(QStringList d) const;
    void encode(const Event&,QStringList&) const;

private:
     enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
};


VEventType::VEventType() : VAttributeType("event")
{
    dataCount_=3;
    searchKeyToData_["event_name"]=NameIndex;
    searchKeyToData_["event_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
}

QString VEventType::toolTip(QStringList d) const
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

void VEventType::encode(const Event& e,QStringList& data) const
{
    data << qName_ <<
              QString::fromStdString(e.name_or_number()) <<
              QString::number((e.value()==true)?1:0);
}

static VEventType atype;

//=====================================================
//
// VEvent
//
//=====================================================

VEvent::VEvent(VNode *parent,const Event& e, int index) : VAttribute(parent,index)
{
    name_=e.name_or_number();
}

VAttributeType* VEvent::type() const
{
    return &atype;
}

QStringList VEvent::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<Event>& v=node->events();
        atype.encode(v[index_],s);
    }
    return s;
}

void VEvent::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<Event>& v=node->events();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VEvent(vnode,v[i],i));
        }
    }
}
