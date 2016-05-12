//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VATTRIBUTE_HPP
#define VATTRIBUTE_HPP

#include "VItem.hpp"

#include <QStringList>
#include <string>
#include <vector>

class VAttributeType;
class VNode;

class VAttribute : public VItem
{
public:
    VAttribute(VNode* parent,int index);
    VAttribute(VNode *parent,VAttributeType* type,QStringList data);

    VAttribute* isAttribute() const {return const_cast<VAttribute*>(this);}
    VAttributeType* type() const {return type_;}
    QStringList data() const {return data_;}
    QString toolTip() const;
    QString name() const;
    std::string strName() const;
    bool isValid(VNode* parent);

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& name,const std::string& value);

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& value);
protected:
    VAttributeType* type_;
    QStringList data_;
    int index_;
};


#endif // VATTRIBUTE_HPP

