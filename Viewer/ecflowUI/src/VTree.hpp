/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VTree_HPP
#define ecflow_viewer_VTree_HPP

#include <vector>

#include <QStringList>

class VNode;
class VAttribute;
class VAttributeType;
class AttributeFilter;
class VTreeServer;
class VTree;

class VTreeNode {
    friend class VTree;

public:
    VTreeNode(VNode* vnode, VTreeNode* parent);
    virtual ~VTreeNode();

    VNode* vnode() const { return vnode_; }
    void addChild(VTreeNode*);
    int numOfChildren() const { return children_.size(); }
    VTreeNode* findChild(const std::string&) const;
    int indexOfChild(const VTreeNode* vn) const;
    int indexInParent() const;
    VTreeNode* childAt(int i) const { return children_[i]; }
    VTreeNode* parent() const { return parent_; }
    virtual VTree* root() const;
    virtual VTreeServer* server() const;

#if 0
    int attrRow(int row,AttributeFilter *filter) const;
#endif
    int attrNum(AttributeFilter* filter) const;
    bool isAttrInitialised() const;
    void updateAttrNum(AttributeFilter* filter = nullptr);
    void resetAttrNum();

    virtual bool isTopLevel() const {
        if (parent_) {
            return (parent_->parent()) ? false : true;
        }
        return false;
    }
    virtual bool isRoot() const { return false; }
    virtual int totalNumOfChildren() const;

public:
    VNode* vnode_;
    std::vector<VTreeNode*> children_;
    VTreeNode* parent_;
    mutable short attrNum_;

protected:
    virtual void countChildren() const;
    void countChildren(int&) const;
};

class VTreeSuiteNode : public VTreeNode {
    friend class VTree;

public:
    VTreeSuiteNode(VNode* vnode, VTreeNode* parent);
    int totalNumOfChildren() const override;

protected:
    void countChildren() const override;
    mutable int num_;
};

class VTree : public VTreeNode {
    friend class VTreeServer;

public:
    explicit VTree(VTreeServer*);
    ~VTree() override;

    VTreeNode* find(const VNode*) const;
    VTree* root() const override;
    VTreeServer* server() const override { return server_; }

    VTreeNode* findAncestor(const VNode*);
    bool isTopLevel() const override { return false; }
    bool isRoot() const override { return true; }
    int totalNum() const { return totalNum_; }
    int totalNumOfTopLevel(VTreeNode*) const;
    int totalNumOfTopLevel(int i) const;
    int indexOfTopLevel(VTreeNode*) const;
    int indexOfTopLevelToInsert(VNode* suite) const;
    VNode* vnodeAt(int index) const;
    const std::vector<VTreeNode*>& nodeVec() const { return nodeVec_; }

protected:
    void clear();
    void build(const std::vector<VNode*>& filter);
    void build();
    void removeChildren(VTreeNode*);
    void remove(VTreeNode*);
    VTreeNode* makeBranch(const std::vector<VNode*>& filter, VTreeNode* parentNode);
    void replaceWithBranch(VTreeNode* node, VTreeNode* branch);
    VTreeNode* makeTopLevelBranch(const std::vector<VNode*>& filter, VNode* suite);
    void insertTopLevelBranch(VTreeNode* branch, int index);

private:
    bool build(VTreeNode* parent, VNode* vnode, const std::vector<VNode*>& filter);
    void build(VTreeNode* parent, VNode* vnode);

    VTreeServer* server_;
    std::vector<VTreeNode*> nodeVec_;
    int totalNum_;
};

#endif /* ecflow_viewer_VTree_HPP */
