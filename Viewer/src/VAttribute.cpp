//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include "VNode.hpp"

#include <QDebug>

//#define  _UI_VATTRIBUTE_DEBUG

VAttribute::VAttribute(VNode* parent,int index) : VItem(parent),
    type_(0), index_(index), id_(-1)
{
    data_=parent_->getAttributeData(index_,type_) ;
}

VAttribute::VAttribute(VNode *parent,VAttributeType* type,QStringList data,int indexInType) :
    VItem(parent),
    type_(type),
    data_(data),
    index_(-1)
{
    id_=indexToId(type_,indexInType);
}        

VServer* VAttribute::root() const
{
    return (parent_)?parent_->root():NULL;
}

QString VAttribute::toolTip() const
{
    return (type_)?(type_->toolTip(data_)):QString();
}

const std::string& VAttribute::typeName() const
{
    static std::string e;
    return (type_)?(type_->strName()):e;
}

std::string VAttribute::fullPath() const
{
    return (parent_)?(parent_->fullPath() + ":" + strName()):"";
}

bool VAttribute::sameContents(VItem* item) const
{
    if(VAttribute *a=item->isAttribute())
    {    return a->parent() == parent() && a->type_ == type_ &&
           a->index_ == index_ && a->strName() == strName();
    }
    return false;
}

int VAttribute::absIndex(AttributeFilter *filter) const
{
    return VAttributeType::absIndexOf(this,filter);
}

void VAttribute::buildAlterCommand(std::vector<std::string>& cmd,
                                    const std::string& action, const std::string& type,
                                    const std::string& name,const std::string& value)
{
    cmd.push_back("ecflow_client");
    cmd.push_back("--alter");
    cmd.push_back(action);
    cmd.push_back(type);

    if(!name.empty())
    {
        cmd.push_back(name);
        cmd.push_back(value);
    }

    cmd.push_back("<full_name>");

}

void VAttribute::buildAlterCommand(std::vector<std::string>& cmd,
                                    const std::string& action, const std::string& type,
                                    const std::string& value)
{
    cmd.push_back("ecflow_client");
    cmd.push_back("--alter");
    cmd.push_back(action);
    cmd.push_back(type);
    cmd.push_back(value);

    cmd.push_back("<full_name>");
}

QString VAttribute::name() const
{
    if(data_.count() >= 2)
       return data_[1];

    return QString();
}

std::string VAttribute::strName() const
{
    return name().toStdString();
}

bool VAttribute::isValid(VNode* parent)
{
    if(type_)
    {
        return type_->exists(parent,data_);
    }

    return false;
}

bool VAttribute::value(const std::string& key,std::string& val) const
{
    if(data_.isEmpty() || !type_)
        return false;

    int idx=type_->searchKeyToDataIndex(key);

#ifdef _UI_VATTRIBUTE_DEBUG
    qDebug() << QString::fromStdString(key) << QString::fromStdString(val);
    qDebug() << "  data=" << data_;
    qDebug() << "  idx=" << idx;
#endif

    if(idx != -1)
    {
        val=data_[idx].toStdString();
        return true;
    }
    return false;
}

VAttribute* VAttribute::make(VNode* n,const std::string& type,const std::string& name)
{
    VAttributeType *t=VAttributeType::find(type);
    assert(t);
    return t->getSearchData(n,name);
}

VAttribute* VAttribute::makeFromId(VNode* n,int id)
{
    if(id ==-1) return NULL;
    VAttributeType *t=idToType(id);
    assert(t);
    int idx=idToTypeIndex(id);
    return t->getSearchData(n,idx);
}

int VAttribute::indexToId(VAttributeType* t,int idx)
{
    return (idx >=0)?(t->id()*10000+idx):-1;
}

VAttributeType* VAttribute::idToType(int id)
{
    if(id < 0) return NULL;
    return VAttributeType::find(id/10000);
}

int VAttribute::idToTypeIndex(int id)
{
    if(id < 0) return -1;
    return id-(id/10000)*10000;
}


