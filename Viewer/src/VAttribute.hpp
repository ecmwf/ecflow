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

#include <boost/shared_ptr.hpp>

class AttributeFilter;
class VAttributeType;
class VNode;

class Event;
class Label;
class Meter;


class VAttribute;
typedef boost::shared_ptr<VAttribute> VAttribute_ptr;

class VAttribute : public VItem
{
public:
    //VAttribute(VNode *parent,VAttributeType* type,int indexInType);
    VAttribute(VNode *parent,int index);

    ~VAttribute();

    virtual VAttributeType* type() const=0;
    virtual const std::string& subType() const;
    virtual int lineNum() const {return 1;}

    VAttribute* clone() const;
    VServer* root() const;
    VAttribute* isAttribute() const {return const_cast<VAttribute*>(this);}

    virtual QStringList data() const=0;
    QString toolTip() const;
    QString name() const;
    virtual std::string strName() const;
    const std::string& typeName() const;
    std::string fullPath() const;

    bool sameAs(QStringList d) const;

    bool sameContents(VItem*) const;
    //int id() const {return id_;}
    //int absIndex(AttributeFilter *filter) const;

    //bool isValid(VNode* parent,QStringList);
    bool value(const std::string& key,std::string& val) const;

    //static VAttribute* make(VNode* n,const std::string& type,const std::string& name);
    //static VAttribute* makeFromId(VNode*,int);
    //static VAttribute* make(VNode *parent,QStringList data);

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& name,const std::string& value);

    static void buildAlterCommand(std::vector<std::string>& cmd,
                         const std::string& action, const std::string& type,
                         const std::string& value);

    static unsigned int totalNum();

protected:
    //VAttribute(VNode *parent,int id);

    //static int indexToId(VAttributeType* t,int idx);
    //static VAttributeType* idToType(int id);
    //static int idToTypeIndex(int id);

    //int id_;
    //std::string name_;
    int index_;
};

#if 0
class VLabel : public VAttribute
{
public:
    VLabel(VNode *parent,const Label&,int index);

    int lineNum() const;
    VAttributeType* type() const;
    QStringList data() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);

protected:
    static void encode(const Label& label,QStringList& data);

    static VAttributeType* type_;
};
#endif

#if 0

class VMeter : public VAttribute
{
public:
    VMeter(VNode *parent,const Meter&,int index);
    VAttributeType* type() const;
    QStringList data() const;
    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);

protected:
    //static void encode(const Meter& m,QStringList& data);

    //static VAttributeType* type_;
};

class VEvent : public VAttribute
{
public:
    VEvent(VNode *parent,const Event&,int index);
    VAttributeType* type() const;
    QStringList data() const;
    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);

protected:
    //static void encode(const Event& e,QStringList& data);
    //static void setType(VAttributeType*);

    //static VAttributeType* type_;
};
#endif


#endif // VATTRIBUTE_HPP

