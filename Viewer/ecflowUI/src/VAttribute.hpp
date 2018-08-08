//============================================================================
// Copyright 2009-2017 ECMWF.
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

class AttributeFilter;
class VAttributeType;
class VNode;
class VAttribute;

class VAttribute : public VItem
{
public:
    VAttribute(VNode *parent,int index);
    ~VAttribute() override;

    virtual VAttributeType* type() const=0;
    virtual const std::string& subType() const;
    virtual int lineNum() const {return 1;}
    ServerHandler* server() const override;
    VServer* root() const override;
    VAttribute* isAttribute() const override {return const_cast<VAttribute*>(this);}
    QString toolTip() const;
    QString name() const override;
    std::string strName() const override;
    const std::string& typeName() const override;
    std::string fullPath() const override;
    virtual QStringList data(bool firstLine=false) const=0;
    QString definition() const;
    bool sameAs(QStringList d) const;
    bool sameContents(VItem*) const override;
    bool value(const std::string& key,std::string& val) const;

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& name,const std::string& value);

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& value);


protected:

    int index_;
};

#endif // VATTRIBUTE_HPP

