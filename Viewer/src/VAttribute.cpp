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
#include "UIDebug.hpp"

#include <QDebug>

//#define  _UI_VATTRIBUTE_DEBUG

static unsigned int totalAttrNum=0;

VAttribute::VAttribute(VNode *parent,VAttributeType* type,int indexInType) :
    VItem(parent)
{
    UI_ASSERT(indexInType >=0, "Index = " << UIDebug::longToString(indexInType));
    assert(type);
    id_=indexToId(type,indexInType);
    totalAttrNum++;
}        

VAttribute::VAttribute(VNode *parent,int id) :
    VItem(parent),
    id_(id)
{
    totalAttrNum++;
}

VAttribute::~VAttribute()
{
    totalAttrNum--;
}

VAttribute* VAttribute::clone() const
{
    return new VAttribute(parent_,id_);
}

unsigned int VAttribute::totalNum()
{
    return totalAttrNum;
}

VServer* VAttribute::root() const
{
    return (parent_)?parent_->root():NULL;
}

QString VAttribute::toolTip() const
{
    VAttributeType* t=type();
    return (t)?(t->toolTip(data())):QString();
}

VAttributeType* VAttribute::type() const
{
    return idToType(id_);
}

const std::string& VAttribute::typeName() const
{
    VAttributeType* t=type();
    static std::string e;
    return (t)?(t->strName()):e;
}

std::string VAttribute::fullPath() const
{
    return (parent_)?(parent_->fullPath() + ":" + strName()):"";
}

bool VAttribute::sameContents(VItem* item) const
{
    if(!item)
        return false;

    if(VAttribute *a=item->isAttribute())
    {    return a->parent() == parent() && a->id_ == id_;
    }
    return false;
}

QStringList VAttribute::data() const
{
    QStringList d;
    if(id_ ==-1) return d;
    VAttributeType *t=idToType(id_);
    assert(t);
    int idx=idToTypeIndex(id_);
    t->itemData(parent(),idx,d);
    return d;
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
    std::string s;
    value("name",s);
    return QString::fromStdString(s);

#if 0
    QStringList d=data();
    if(d.count() >= 2)
       return d[1];

    return QString();
#endif
}

std::string VAttribute::strName() const
{
    return name().toStdString();
}

bool VAttribute::isValid(VNode* parent,QStringList data)
{
    if(VAttributeType* t=type())
    {
        return t->exists(parent,data);
    }
    return false;
}

bool VAttribute::value(const std::string& key,std::string& val) const
{
    QStringList d=data();
    VAttributeType* t=type();
    if(d.isEmpty() || !t)
        return false;

    int idx=t->searchKeyToDataIndex(key);

#ifdef _UI_VATTRIBUTE_DEBUG
    qDebug() << QString::fromStdString(key) << QString::fromStdString(val);
    qDebug() << "  data=" << d;
    qDebug() << "  idx=" << idx;
#endif

    if(idx != -1)
    {
        val=d[idx].toStdString();
        return true;
    }
    return false;
}

VAttribute* VAttribute::make(VNode* n,const std::string& type,const std::string& name)
{
    if(!n) return NULL;
    VAttributeType *t=VAttributeType::find(type);
    assert(t);
    VItemTmp_ptr item=t->item(n,name);
    return (item)?item->attribute()->clone():NULL;
}

VAttribute* VAttribute::makeFromId(VNode* n,int id)
{
    if(id ==-1) return NULL;
    VAttributeType *t=idToType(id);
    assert(t);
    QStringList d;
    int idx=idToTypeIndex(id);
    return NULL;
    //return t->getSearchData(n,idx,d);
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


