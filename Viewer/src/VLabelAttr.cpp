//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VLabelAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VLabelAttrType
//================================

class VLabelAttrType : public VAttributeType
{
public:
    explicit VLabelAttrType();
    QString toolTip(QStringList d) const;
    void encode(const Label& label,QStringList& data) const;

private:
    enum DataIndex {TypeIndex=0,NameIndex=1,ValueIndex=2};
};


VLabelAttrType::VLabelAttrType() : VAttributeType("label")
{
    dataCount_=3;
    searchKeyToData_["label_name"]=NameIndex;
    searchKeyToData_["label_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
}

QString VLabelAttrType::toolTip(QStringList d) const
{
    QString t="<b>Type:</b> Label<br>";
    if(d.count() == dataCount_)
    {
        t+="<b>Name:</b> " + d[NameIndex] + "<br>";
        t+="<b>Value:</b> " + d[ValueIndex];
    }
    return t;
}

void VLabelAttrType::encode(const Label& label,QStringList& data) const
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

static VLabelAttrType atype;

//=====================================================
//
// VLabelAttr
//
//=====================================================

VLabelAttr::VLabelAttr(VNode *parent,const Label& label, int index) : VAttribute(parent,index)
{
    name_=label.name();
}

int VLabelAttr::lineNum() const
{
    return parent_->labelLineNum(index_);
}

VAttributeType* VLabelAttr::type() const
{
    return &atype;
}

QStringList VLabelAttr::data() const
{
    QStringList s;
    if(node_ptr node=parent_->node())
    {
        const std::vector<Label>& v=node->labels();
        atype.encode(v[index_],s);
    }
    return s;
}

void VLabelAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(node_ptr node=vnode->node())
    {
        const std::vector<Label>& v=node->labels();
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VLabelAttr(vnode,v[i],i));
        }
    }
}
