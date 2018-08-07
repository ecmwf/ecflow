//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VAttributeType.hpp"

#include <QDebug>

#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <map>

#include "VNode.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "VFilter.hpp"
#include "VAttribute.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "UIDebug.hpp"

std::map<std::string,VAttributeType*> VAttributeType::typesMap_;
std::vector<VAttributeType*> VAttributeType::types_;

//#define _UI_ATTR_DEBUG

VAttributeType::VAttributeType(const std::string& name) :
        VParam(name),
        dataCount_(0),
        typeId_(types_.size())
{
    typesMap_[name]=this;
    types_.push_back(this);
}

std::vector<VParam*> VAttributeType::filterItems()
{
    std::vector<VParam*> v;
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
        v.push_back(*it);

    return v;
}

VAttributeType* VAttributeType::find(const std::string& name)
{
    std::map<std::string,VAttributeType*>::const_iterator it=typesMap_.find(name);
    if(it != typesMap_.end())
        return it->second;

    return 0;
}

VAttributeType* VAttributeType::find(int id)
{
    assert(id >=0  && id < static_cast<int>(types_.size()));
    return types_[id];
}

void VAttributeType::scan(VNode* vnode,std::vector<VAttribute*>& v)
{
    for(TypeIterator it=types_.begin(); it != types_.end(); ++it)
    {
        (*it)->scanProc()(vnode,v);
    }
}

int VAttributeType::keyToDataIndex(const std::string& key) const
{
    std::map<std::string,int>::const_iterator it=keyToData_.find(key);
    if(it != keyToData_.end())
        return it->second;

    return -1;
}

int VAttributeType::searchKeyToDataIndex(const std::string& key) const
{
    std::map<std::string,int>::const_iterator it=searchKeyToData_.find(key);
    if(it != searchKeyToData_.end())
        return it->second;

    return -1;
}

QStringList VAttributeType::searchKeys() const
{
    QStringList lst;
    for(std::map<std::string,int>::const_iterator it=searchKeyToData_.begin(); it != searchKeyToData_.end(); it++)
    {
        lst << QString::fromStdString(it->first);
    }
    return lst;
}

//Load the attributes parameter file
void VAttributeType::load(VProperty* group)
{
    //We set some extra information on each type and also
    //try to reorder the types according to the order defined in the
    //parameter file. This order is very important:
    // -it defines the rendering order
    // -defines the order of the attribute items in the attribute filter
    std::vector<VAttributeType*> v;
    Q_FOREACH(VProperty* p,group->children())
    {
         if(VAttributeType* obj=VAttributeType::find(p->strName()))
         {
            obj->setProperty(p);
            v.push_back(obj);
         }
         else
         {
             UserMessage::message(UserMessage::ERROR,true,
                        "Unknown attribute type=" + p->strName() + " is loaded from parameter file!");
             exit(1);
             //UI_ASSERT(0,"Unknown attribute type is read from parameter file: " << p->strName());
         }
    }

    //UI_ASSERT(v.size() == types_.size(),"types size=" << types_.size() << "loaded size=" << v.size());

    if(v.size() == types_.size())
        types_=v;
    else
    {
        UserMessage::message(UserMessage::ERROR,true,
                   "The number attributes loaded from parameter file do not match expected number! loaded=" +
                   UserMessage::toString(v.size()) + " expected=" + UserMessage::toString(types_.size()));
        exit(1);
    }
}


static SimpleLoader<VAttributeType> loader("attribute");

//Initialise attribute types

#include "VLabelAttr.hpp"
#include "VMeterAttr.hpp"
#include "VEventAttr.hpp"
#include "VLimitAttr.hpp"
#include "VLimiterAttr.hpp"
#include "VRepeatAttr.hpp"
#include "VTriggerAttr.hpp"
#include "VDateAttr.hpp"
#include "VTimeAttr.hpp"
#include "VLateAttr.hpp"
#include "VGenVarAttr.hpp"
#include "VUserVarAttr.hpp"

static VLabelAttrType labelAttrType;
static VMeterAttrType meterAttType;
static VEventAttrType eventAttrType;
static VLimitAttrType limitAttrType;
static VLimiterAttrType limiterAttrType;
static VRepeatAttrType repeatAttrType;
static VTriggerAttrType triggerAttrType;
static VDateAttrType dateAttrType;
static VTimeAttrType timeAttrType;
static VLateAttrType lateAttrType;
static VGenVarAttrType genvarAttrType;
static VUserVarAttrType uservarAttrType;
