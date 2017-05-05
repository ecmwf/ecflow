//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VTriggerAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VTriggerAttrType
//================================

class VTriggerAttrType : public VAttributeType
{
public:
    explicit VTriggerAttrType();
    QString toolTip(QStringList d) const;
    void encodeTrigger(Expression*,QStringList&) const;
    void encodeComplete(Expression*,QStringList&) const;

private:
    enum DataIndex {TypeIndex=0,CompleteIndex=1,ExprIndex=2};
};


VTriggerAttrType::VTriggerAttrType() : VAttributeType("trigger")
{
    dataCount_=3;
    searchKeyToData_["trigger_type"]=CompleteIndex;
    searchKeyToData_["trigger_expression"]=ExprIndex;
    searchKeyToData_["name"]=TypeIndex;
    scanProc_=VTriggerAttr::scan;
}

QString VTriggerAttrType::toolTip(QStringList d) const
{
    QString t;
    if(d.count() == dataCount_)
    {
        if(d[CompleteIndex] == "0")
            t+="<b>Type:</b> Trigger<br>";
        else if(d[CompleteIndex] == "1")
            t+="<b>Type:</b> Complete<br>";
        else
            return t;

        t+="<b>Expression:</b> " + d[ExprIndex];
    }
    return t;
}

void VTriggerAttrType::encodeTrigger(Expression *e,QStringList& data) const
{
    data << qName_ << "0" << QString::fromStdString(e->expression());
}

void VTriggerAttrType::encodeComplete(Expression *e,QStringList& data) const
{
    data << qName_ << "1" << QString::fromStdString(e->expression());
}

static VTriggerAttrType atype;

//=====================================================
//
// VTriggerAttr
//
//=====================================================

VTriggerAttr::VTriggerAttr(VNode *parent,Expression* e, int index) :
    VAttribute(parent,index)
{
}

VAttributeType* VTriggerAttr::type() const
{
    return &atype;
}

QStringList VTriggerAttr::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        if(index_ == 0)
        {
            atype.encodeTrigger(node->get_trigger(),s);
        }
        else
        {
            atype.encodeComplete(node->get_complete(),s);
        }
    }
    return s;
}

std::string VTriggerAttr::strName() const
{
    return std::string("trigger");
}

void VTriggerAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        Expression* eT=node->get_trigger();
        Expression* eC=node->get_complete();

        if(eT)
            vec.push_back(new VTriggerAttr(vnode,eT,0));

        if(eC)
            vec.push_back(new VTriggerAttr(vnode,eC,1));
    }
}

void VTriggerAttr::expressions(const VNode* vnode,std::string& trigger, std::string& complete)
{
    if(node_ptr node=vnode->node())
    {
        Expression* eT=node->get_trigger();
        Expression* eC=node->get_complete();

        if(eT)
            trigger=eT->expression();

        if(eC)
            complete=eC->expression();
    }
}

