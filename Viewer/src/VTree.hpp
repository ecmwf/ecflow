//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VTREENODE_HPP
#define VTREENODE_HPP

#include <vector>

#include <QStringList>

class VNode;
class VAttributeType;
class AttributeFilter;
class VTreeServer;
class VTree;

class VTreeNode
{
public:
    VTreeNode(VNode* vnode,VTreeNode* parent);
    virtual ~VTreeNode();

    VNode* vnode() const {return vnode_;}
    void addChild(VTreeNode*);
    int numOfChildren() const {return children_.size();}
    VTreeNode* findChild(const std::string&) const;
    int indexOfChild(const VTreeNode* vn) const;
    int indexInParent() const;
    VTreeNode* childAt(int i) {return children_[i];}
    VTreeNode* parent() const {return parent_;}
    virtual VTree* root() const;
    virtual VTreeServer* server() const;

    int attrNum(AttributeFilter* filter=0) const;
    bool isAttrInitialised() const;
    void updateAttrNum(AttributeFilter* filter=0);
    QStringList getAttributeData(int row,VAttributeType*& type,AttributeFilter *filter=0);
    int getAttributeLineNum(int row,AttributeFilter *filter=0);
    void resetAttrNum();

    virtual bool isTopLevel() const {if(parent_) return (parent_->parent())?false:true; return false;}
    virtual bool isRoot() const {return false;}

public:
    VNode* vnode_;
    std::vector<VTreeNode*> children_;
    VTreeNode *parent_;
    mutable short attrNum_;
};

class VTree : public VTreeNode
{
friend class VTreeServer;

public:
     VTree(VTreeServer*);
     ~VTree();

     VTreeNode* find(const VNode*) const;
     VTree* root() const;
     VTreeServer* server() const {return server_;}

     VTreeNode *findAncestor(const VNode*);
     bool isTopLevel() const {return false;}
     bool isRoot() const {return true;}
     int totalNum() const {return totalNum_;}
     int totalNumOfTopLevel(VTreeNode*) const;
     int totalNumOfTopLevel(int i) const;
     VNode* vnodeAt(int index) const;

     const std::vector<VTreeNode*>& nodeVec() const {return nodeVec_;}

protected:
     void clear();
     void build(const std::vector<VNode*>& filter);
     void build();
     void removeChildren(VTreeNode*);
     void buildBranch(const std::vector<VNode*>& filter,VTreeNode* node,VTreeNode* branch);
     void addBranch(VTreeNode* node,VTreeNode* branch);

private:
     void build(VTreeNode* parent,VNode* vnode,const std::vector<VNode*>& filter);
     void build(VTreeNode* parent,VNode* vnode);

     VTreeServer* server_;
     std::vector<VTreeNode*> nodeVec_;
     int totalNum_;
     std::vector<int> totalNumInChild_;
};

#endif // VTREENODE_HPP

