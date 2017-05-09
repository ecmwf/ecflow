//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VGenVarAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include "NodeAttr.hpp"

//================================
// VGenVarAttrType
//================================

VGenVarAttrType::VGenVarAttrType() : VAttributeType("genvar")
{
    dataCount_=3;
    searchKeyToData_["var_name"]=NameIndex;
    searchKeyToData_["var_value"]=ValueIndex;
    searchKeyToData_["var_type"]=TypeIndex;
    searchKeyToData_["name"]=NameIndex;
    scanProc_=VGenVarAttr::scan;
}

QString VGenVarAttrType::toolTip(QStringList d) const
{
   return QString();
}

void VGenVarAttrType::encode(const Variable& v,QStringList& data) const
{
    data << qName_ <<
            QString::fromStdString(v.name()) <<
            QString::fromStdString(v.theValue());
}

//=====================================================
//
// VGenVarAttr
//
//=====================================================

VGenVarAttr::VGenVarAttr(VNode *parent,const Variable& v, int index) : VAttribute(parent,index)
{
    //name_=v.name();
}

VAttributeType* VGenVarAttr::type() const
{
    static VAttributeType* atype=VAttributeType::find("genvar");
    return atype;
}

QStringList VGenVarAttr::data(bool /*firstLine*/) const
{
    static VGenVarAttrType* atype=static_cast<VGenVarAttrType*>(type());
    QStringList s;
    if(parent_->isServer() == 0)
    {
        std::vector<Variable> v;
        parent_->genVariables(v);
        atype->encode(v[index_],s);
    }
    return s;
}

std::string VGenVarAttr::strName() const
{
    if(parent_->isServer() == 0)
    {
        std::vector<Variable> v;
        parent_->genVariables(v);
        return v[index_].name();
    }
    return std::string();
}

void VGenVarAttr::scan(VNode* vnode,std::vector<VAttribute*>& vec)
{
    if(vnode->isServer() == 0)
    {
        std::vector<Variable> v;
        vnode->genVariables(v);
        int n=v.size();
        for(size_t i=0; i < n; i++)
        {
            vec.push_back(new VGenVarAttr(vnode,v[i],i));
        }
    }
}

