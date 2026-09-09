/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VItem_HPP
#define ecflow_viewer_VItem_HPP

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

class VItem {
public:
    explicit VItem(VNode* parent)
        : parent_(parent) {}
    virtual ~VItem() = default;

    VNode* parent() const { return parent_; }
    virtual VServer* isServer() const { return nullptr; }
    virtual VNode* isNode() const { return nullptr; }
    virtual VSuiteNode* isSuite() const { return nullptr; }
    virtual VFamilyNode* isFamily() const { return nullptr; }
    virtual VTaskNode* isTask() const { return nullptr; }
    virtual VAliasNode* isAlias() const { return nullptr; }
    virtual VAttribute* isAttribute() const { return nullptr; }

    virtual ServerHandler* server() const = 0;
    virtual VServer* root() const         = 0;
    virtual bool isTopLevel() const { return false; }
    virtual std::string strName() const         = 0;
    virtual QString name() const                = 0;
    virtual const std::string& typeName() const = 0;
    virtual std::string fullPath() const        = 0;
    virtual bool sameContents(VItem*) const     = 0;
    virtual bool isAncestor(const VItem*) const;
    virtual QString nodeMenuMode() const { return QString(); }
    virtual QString defStatusNodeMenuMode() const { return QString(); }

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

#endif /* ecflow_viewer_VItem_HPP */
