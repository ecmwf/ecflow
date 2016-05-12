//============================================================================
// Copyright 2014 ECMWF.
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

VAttribute::VAttribute(VNode* parent,int index) : VItem(parent), type_(0), index_(index)
{
    data_=parent_->getAttributeData(index_,type_) ;
}

VAttribute::VAttribute(VNode *parent,VAttributeType* type,QStringList data) : 
    VItem(parent),
    type_(type),
    data_(data),
    index_(-1)
{
}        

QString VAttribute::toolTip() const
{
    return (type_)?(type_->toolTip(data_)):QString();
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
