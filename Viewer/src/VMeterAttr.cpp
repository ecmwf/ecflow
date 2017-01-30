//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VMeterAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VMeterAttrType
//================================

class VMeterAttrType : public VAttributeType
{
public:
    explicit VMeterAttrType();
    QString toolTip(QStringList d) const;
    void encode(const Meter&,QStringList&) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2,MinIndex=3, MaxIndex=4,ThresholdIndex=5};
};


VMeterAttrType::VMeterAttrType() : VAttributeType("meter")
{
    dataCount_=6;
    searchKeyToData_["meter_name"]=NameIndex;
    searchKeyToData_["meter_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
    scanProc_=VMeterAttr::scan;
}

QString VMeterAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Meter<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Value:</b> " + d[ValueIndex]+ "<br>";
        t+="<b>Minimum:</b> " + d[MinIndex] + "<br>";
        t+="<b>Maximum:</b> " + d[MaxIndex];
    }
    return t;
}

void VMeterAttrType::encode(const Meter& m,QStringList& data) const
{
    data << qName_ <<
                    QString::fromStdString(m.name()) <<
                    QString::number(m.value()) << QString::number(m.min()) << QString::number(m.max()) <<
                    QString::number(m.colorChange());
}

static VMeterAttrType atype;

//=====================================================
//
// VMeterAttr
//
//=====================================================

VMeterAttr::VMeterAttr(VNode *parent,const Meter& m, int index) : VAttribute(parent,index)
{
    //name_=m.name();
}

VAttributeType* VMeterAttr::type() const
{
    return &atype;
}

QStringList VMeterAttr::data() const
{
    QStringList s;
    if(parent_->node_)
    {
        const std::vector<Meter>& v=parent_->node_->meters();
        atype.encode(v[index_],s);
    }
    return s;
}

std::string VMeterAttr::strName() const
{
    if(parent_->node_)
    {
        const std::vector<Meter>& v=parent_->node_->meters();
        return v[index_].name();
    }
    return std::string();
}

void VMeterAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(vnode->node_)
    {
        const std::vector<Meter>& v=vnode->node_->meters();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VMeterAttr(vnode,v[i],i));
        }
    }
}

int VMeterAttr::totalNum(VNode* vnode)
{
    if(vnode->node_)
    {
        return vnode->node_->meters().size();
    }
    return 0;
}
