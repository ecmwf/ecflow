//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VITEM_HPP_
#define VITEM_HPP_

#include <cstdlib>
#include <QString>

class ServerHandler;
class VNode;
class VServer;
class VSuiteNode;
class VFamilyNode;
class VAliasNode;
class VTaskNode;
class VAttribute;
class VItemVisitor;

class VItem
{
public:
    VItem(VNode* parent) : parent_(parent) {}
    virtual ~VItem() = default;

    VNode* parent() const {return parent_;}
    virtual VServer* isServer() const {return nullptr;}
    virtual VNode* isNode() const {return nullptr;}
    virtual VSuiteNode* isSuite() const {return nullptr;}
    virtual VFamilyNode* isFamily() const {return nullptr;}
    virtual VTaskNode* isTask() const {return nullptr;}
    virtual VAliasNode* isAlias() const {return nullptr;}
    virtual VAttribute* isAttribute() const {return nullptr;}

    virtual ServerHandler* server() const=0;
    virtual VServer* root() const=0;
    virtual bool isTopLevel() const {return false;}
    virtual std::string strName() const=0;
    virtual QString name() const=0;
    virtual const std::string& typeName() const=0;
    virtual std::string fullPath() const=0;
    virtual bool sameContents(VItem*) const=0;
    virtual bool isAncestor(const VItem*) const;
    virtual QString nodeMenuMode() const {return QString();}
    virtual QString defStatusNodeMenuMode() const {return QString();}

protected:
    VNode* parent_;
};


#if 0
class VItemVisitor
{
public:
    VItemVisitor() {}
    virtual ~VItemVisitor() {}

    virtual void visit(VServer*) {}
    virtual void visit(VNode*) {}
    virtual void visit(VAttribute*) {}
};
#endif


#endif // VITEM_HPP_

