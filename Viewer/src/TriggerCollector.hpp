//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERCOLLECTOR_HPP
#define TRIGGERCOLLECTOR_HPP

#include <string>
#include <vector>

#include <stdio.h>

class TriggerListItem;

#include "VItemTmp.hpp"
#include "VNode.hpp"

class TriggerCollector
{
public:
    TriggerCollector() {}
    virtual ~TriggerCollector() {}

    enum Mode { Normal    = 0,   // Normal trigger_node
                Parent    = 1,   // Through parent
                Child     = 2,   // Through child
                Hierarchy = 3    // Through child
    };

    //virtual bool add(VItem*, VItem*,Mode,VItem*) = 0;
    virtual bool add(VItemTmp_ptr, VItemTmp_ptr,Mode) = 0;
    virtual bool scanParents() { return false; }
    virtual bool scanKids()    { return false; }
    virtual bool scanSelf()    { return true; }

private:
    TriggerCollector(const TriggerCollector&);
    TriggerCollector& operator=(const TriggerCollector&);
};

class TriggerListCollector : public TriggerCollector
{
public:
    TriggerListCollector(FILE* f,const std::string& title,bool extended) :
        file_(f), title_(title), extended_(extended) {}

    ~TriggerListCollector();
    bool add(VItemTmp_ptr, VItemTmp_ptr,Mode);
    bool scanParents() { return extended_; }
    bool scanKids() { return extended_; }

    const std::vector<TriggerListItem*>& items() const {return items_;}

protected:   
    FILE* file_;
    std::string title_;
    bool extended_;
    std::vector<TriggerListItem*> items_;
};

class TriggerChildCollector : public TriggerCollector
{
public:
    TriggerChildCollector(VItem *n,VItem* child,TriggerCollector* collector) :
        node_(VItemTmp::create(n)), child_(VItemTmp::create(child)), collector_(collector) {}

    ~TriggerChildCollector();
    bool add(VItemTmp_ptr, VItemTmp_ptr,Mode);

private:
  VItemTmp_ptr node_;
  VItemTmp_ptr child_;
  TriggerCollector* collector_;
};

class TriggerParentCollector : public TriggerCollector
{
public:
    TriggerParentCollector(VItem* parent,TriggerCollector* collector) :
        parent_(VItemTmp::create(parent)), collector_(collector) {}
    ~TriggerParentCollector();

    bool add(VItemTmp_ptr, VItemTmp_ptr,Mode);

private:
  VItemTmp_ptr parent_;
  TriggerCollector* collector_;
};

class TriggeredCollector : public TriggerListCollector
{
public:
    TriggeredCollector(VNode* n) :
        TriggerListCollector(NULL,"",false), node_(VItemTmp::create(n)) {}
    bool add(VItemTmp_ptr, VItemTmp_ptr,Mode);

private:
  VItemTmp_ptr node_;
};

class TriggerListItem
{
public:
    TriggerListItem(VItemTmp_ptr t,VItemTmp_ptr dep,TriggerCollector::Mode mode) :
        t_(t), dep_(dep), mode_(mode) {}

    ~TriggerListItem();

    VItem* item() const {return (t_)?t_->item():NULL;}
    VItem* dep()  const {return (dep_)?dep_->item():NULL;}
    TriggerCollector::Mode mode() const {return mode_;}

protected:
    VItemTmp_ptr t_; //trigger or triggered
    VItemTmp_ptr dep_;
    TriggerCollector::Mode mode_;
};


#endif // TRIGGERCOLLECTOR_HPP

