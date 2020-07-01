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

VLabelAttrType::VLabelAttrType() : VAttributeType("label")
{
    dataCount_=3;
    searchKeyToData_["label_name"]=NameIndex;
    searchKeyToData_["label_value"]=ValueIndex;
    searchKeyToData_["name"]=NameIndex;
    scanProc_=VLabelAttr::scan;
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

QString VLabelAttrType::definition(QStringList d) const
{
    QString t="label";
    if(d.count() == dataCount_)
    {
        t+=" " + d[NameIndex] + " '" + d[ValueIndex] + "'";
    }
    return t;
}

void VLabelAttrType::encode(const Label& label,QStringList& data,bool firstLine) const
{
    std::string r;
    const std::string& val=label.new_value();
    if(val.empty() || val == " ")
    {
        VLabelAttr::getLines(label.value(), r, firstLine);
    } else {
        VLabelAttr::getLines(val, r, firstLine);
    }


//    if(firstLine)
//    {
//        static const char ch_break = '\n';
        std::size_t pos=val.find('\n');
//        if(pos != std::string::npos)
//        {
//            val=val.substr(0,pos);
//        }
//    }

    data << qName_ <<
                QString::fromStdString(label.name()) <<
                QString::fromStdString(r);

}

void VLabelAttrType::encode_empty(QStringList& data) const
{
    data << qName_ << "" << "";
}

//=====================================================
//
// VLabelAttr
//
//=====================================================

VLabelAttr::VLabelAttr(VNode *parent,const Label& label, int index) : VAttribute(parent,index)
{
    //name_=label.name();
}

int VLabelAttr::lineNum() const
{
    if(parent_->node_)
    {
        const std::vector<Label>&  v=parent_->node_->labels();
        const std::string& val=v[index_].new_value();
        if(val.empty() || val == " ")
        {
            return countLineNum(v[index_].value());
        } else {
            return countLineNum(val);
        }
    }

    return 1;
}

int VLabelAttr::countLineNum(const std::string& s)
{
    static const char ch_break = '\n';
    return std::count(s.begin(), s.end(), ch_break);
}

void VLabelAttr::getLines(const std::string& s, std::string& r, bool firstLine)
{
    static const char ch_break = '\n';
    if(firstLine) {
        std::size_t pos=s.find(ch_break);
        if(pos != std::string::npos) {
            r=s.substr(0,pos);
            return;
        }
    }
    r = s;
}

VAttributeType* VLabelAttr::type() const
{
    static VAttributeType* atype=VAttributeType::find("label");
    return atype;
}

QStringList VLabelAttr::data(bool firstLine) const
{
    static auto* atype=static_cast<VLabelAttrType*>(type());
    QStringList s;
    if(parent_->node_)
    {
        const std::vector<Label>& v=parent_->node_->labels();
        if (index_ < static_cast<int>(v.size()))
            atype->encode(v[index_],s,firstLine);

        // this can happen temporarily during update when:
        // -an attribute was already deleted
        // -the notification was emmitted from the update thread but
        //  it not yet reached the main thread
        // -here in the main thread we still have the old (incorrect) atribute number
        // as safety measure,in this case we encode an empty attribute. When the
        // notification arrives all the attributes of the given node will be rescanned
        //and we will have a correct state.
        else
            atype->encode_empty(s);
    }
    return s;
}

std::string VLabelAttr::strName() const
{
    if(parent_->node_)
    {
        const std::vector<Label>& v=parent_->node_->labels();
        return v[index_].name();
    }
    return std::string();
}

void VLabelAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(vnode->node_)
    {
        const std::vector<Label>& v=vnode->node_->labels();
        auto n=static_cast<int>(v.size());
        for(int i=0; i < n; i++)
        {
            vec.push_back(new VLabelAttr(vnode,v[i],i));
        }
    }
}
