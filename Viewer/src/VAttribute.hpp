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

#include <boost/shared_ptr.hpp>

class AttributeFilter;
class VAttributeType;
class VNode;

class VAttribute;
typedef boost::shared_ptr<VAttribute> VAttribute_ptr;

class VAttribute : public VItem
{
public:
    VAttribute(VNode *parent,VAttributeType* type,int indexInType);
    ~VAttribute();

    VAttribute* clone();
    VServer* root() const;
    VAttribute* isAttribute() const {return const_cast<VAttribute*>(this);}
    VAttributeType* type() const;
    QStringList data() const;
    QString toolTip() const;
    QString name() const;
    std::string strName() const;
    const std::string& typeName() const;
    std::string fullPath() const;
    bool sameContents(VItem*) const;
    int id() const {return id_;}
    int absIndex(AttributeFilter *filter) const;

    bool isValid(VNode* parent,QStringList);
    bool value(const std::string& key,std::string& val) const;

    static VAttribute* make(VNode* n,const std::string& type,const std::string& name);
    static VAttribute* makeFromId(VNode*,int);

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& name,const std::string& value);

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& value);

    static QString total();

protected:
    VAttribute(VNode *parent,int id);

    static int indexToId(VAttributeType* t,int idx);
    static VAttributeType* idToType(int id);
    static int idToTypeIndex(int id);

    //VAttributeType* type_;
    int id_;
};


#endif // VATTRIBUTE_HPP

