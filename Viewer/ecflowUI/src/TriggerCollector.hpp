//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERCOLLECTOR_HPP
#define TRIGGERCOLLECTOR_HPP

#include <set>
#include <string>
#include <vector>

#include <cstdio>

class TriggerListItem;
class VItem;

#include "VNode.hpp"

class TriggerCollector
{
public:
    TriggerCollector() = default;
    virtual ~TriggerCollector() = default;

    enum Mode { Normal    = 0,   // Normal trigger_node
                Parent    = 1,   // Through parent
                Child     = 2,   // Through child
                Hierarchy = 3    // Through child
    };

    virtual bool add(VItem*, VItem*,Mode) = 0;
    virtual bool scanParents() { return false; }
    virtual bool scanKids()    { return false; }
    virtual bool scanSelf()    { return true; }

private:
    TriggerCollector(const TriggerCollector&) = delete;
    TriggerCollector& operator=(const TriggerCollector&) = delete;
};

class TriggerListCollector : public TriggerCollector
{
public:
    TriggerListCollector(bool extended) :
        extended_(extended) {}

    ~TriggerListCollector() override;
    bool add(VItem*, VItem*,Mode) override;
    bool scanParents() override { return extended_; }
    bool scanKids() override { return extended_; }
    void setDependency(bool);
    void clear();
    size_t size() const {return items_.size();}

    const std::vector<TriggerListItem*>& items() const {return items_;}

protected:
    bool extended_;
    std::vector<TriggerListItem*> items_;
};

class TriggerChildCollector : public TriggerCollector
{
public:
    TriggerChildCollector(VItem *n,VItem* child,TriggerCollector* collector) :
        node_(n), child_(child), collector_(collector) {}

    bool add(VItem*, VItem*,Mode) override;

private:
  VItem* node_;
  VItem* child_;
  TriggerCollector* collector_;
};

class TriggerParentCollector : public TriggerCollector
{
public:
    TriggerParentCollector(VItem* parent,TriggerCollector* collector) :
        parent_(parent), collector_(collector) {}

    bool add(VItem*, VItem*,Mode) override;

private:
  VItem* parent_;
  TriggerCollector* collector_;
};

class TriggeredCollector : public TriggerListCollector
{
public:
    TriggeredCollector(VNode* n) :
        TriggerListCollector(false), node_(n) {}
    bool add(VItem*, VItem*,Mode) override;

private:
  VItem* node_;
};

class TriggerListItem
{
public:
    TriggerListItem(VItem* t,VItem* dep,TriggerCollector::Mode mode) :
        t_(t), dep_(dep), mode_(mode) {}

    VItem* item() const {return t_;}
    VItem* dep()  const {return dep_;}
    TriggerCollector::Mode mode() const {return mode_;}

protected:
    VItem* t_; //trigger or triggered
    VItem* dep_;
    TriggerCollector::Mode mode_;
};

class TriggerDependencyItem
{
public:
    TriggerDependencyItem(VItem* dep,TriggerCollector::Mode mode) :
        dep_(dep), mode_(mode) {}

    VItem* dep()  const {return dep_;}
    TriggerCollector::Mode mode() const {return mode_;}

protected:
    VItem* dep_;
    TriggerCollector::Mode mode_;
};

class TriggerTableItem
{
public:
    TriggerTableItem(VItem* t) :t_(t){}

    void addDependency(VItem* dep,TriggerCollector::Mode mode)
            {deps_.emplace_back(dep,mode);}

    VItem* item() const {return t_;}
    const std::vector<TriggerDependencyItem>& dependencies() const {return deps_;}
    const std::set<TriggerCollector::Mode>& modes() const;

protected:
     VItem* t_; //trigger or triggered
     std::vector<TriggerDependencyItem> deps_;
     mutable std::set<TriggerCollector::Mode> modes_;
};


class TriggerTableCollector : public TriggerCollector
{
public:
    TriggerTableCollector(bool extended) :
        extended_(extended) {}

    ~TriggerTableCollector() override;
    bool add(VItem*, VItem*,Mode) override;
    bool scanParents() override { return extended_; }
    bool scanKids() override { return extended_; }
    void setDependency(bool);
    void clear();
    size_t size() const {return items_.size();}

    bool contains(TriggerTableItem*) const;
    bool contains(const VNode*,bool attrParents=true) const;
    TriggerTableItem* find(const VItem* item) const;
    TriggerTableItem* findByContents(const VItem* item) const;
    const std::vector<TriggerTableItem*>& items() const {return items_;}

protected:
    bool extended_;
    std::vector<TriggerTableItem*> items_;
};

#if 0
class nl1 : public trigger_lister {
    int	    n_;
    graph_layout&	t_;
    node* g_;
    bool e_;
public:

    nl1(graph_layout& t,node* g,bool e) : n_(0), t_(t), g_(g), e_(e) {}

    void next_node(node& n,node* p,int mode,node* t) {
        t_.relation(&n,g_,p,mode,t);
        n_++;
    }

    Boolean parents() { return e_; }
    Boolean kids() { return e_; }

    int count() { return n_; }
};
#endif


#endif // TRIGGERCOLLECTOR_HPP

