//============================================================================
// Copyright 2009-2017 ECMWF.
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


VAttribute::VAttribute(VNode *parent,int index) :
    VItem(parent),
    index_(index)
{
}

VAttribute::~VAttribute()
{
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

const std::string& VAttribute::typeName() const
{
    VAttributeType* t=type();
    assert(t);
    static std::string e;
    return (t)?(t->strName()):e;
}

const std::string& VAttribute::subType() const
{
    static std::string e;
    return e;
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
    {    return a->parent() == parent() &&
                a->type() == type() &&
                name() == name();
    }
    return false;
}

QString VAttribute::name() const
{  
   return QString::fromStdString(strName());
}

std::string VAttribute::strName() const
{
    static std::string eStr;
    return eStr;
}

bool VAttribute::value(const std::string& key,std::string& val) const
{
    int idx=type()->searchKeyToDataIndex(key);
    if(idx != -1)
    {
        QStringList d=data();
        val=d[idx].toStdString();
        return true;
    }
    return false;
}

bool VAttribute::sameAs(QStringList d) const
{
    if(d.count() >=2)
    {
        VAttributeType* t=type();

        if(t->name() == d[0])
        {
            int idx=t->searchKeyToDataIndex("name");
            if(idx != -1 && idx < d.count())
            {
                return name() == d[idx];
            }
        }
    }
    return false;
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

