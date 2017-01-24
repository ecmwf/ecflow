//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VLabel.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VLabelType
//================================

class VLabelType : public VAttributeType
{
public:
    explicit VLabelType();
    QString toolTip(QStringList d) const;
    void encode(const Label& label,QStringList& data) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
};


VLabelType::VLabelType() : VAttributeType("label")
{
    dataCount_=3;
    searchKeyToData_["label_name"]=NameIndex;
    searchKeyToData_["label_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
}

QString VLabelType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Label<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Value:</b> " + d[ValueIndex];
    }
    return t;
}

void VLabelType::encode(const Label& label,QStringList& data) const
{
    std::string val=label.new_value();
    if(val.empty() || val == " ")
    {
        val=label.value();
    }

    data << qName_ <<
                QString::fromStdString(label.name()) <<
                QString::fromStdString(val);
}

static VLabelType atype;

//=====================================================
//
// VLabel
//
//=====================================================

VLabel::VLabel(VNode *parent,const Label& label, int index) : VAttribute(parent,index)
{
    name_=label.name();
}

int VLabel::lineNum() const
{
    return parent_->labelLineNum(index_);
}

VAttributeType* VLabel::type() const
{
    return &atype;
}

QStringList VLabel::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<Label>& v=node->labels();
        atype.encode(v[index_],s);
    }
    return s;
}

void VLabel::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<Label>& v=node->labels();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VLabel(vnode,v[i],i));
        }
    }
}
