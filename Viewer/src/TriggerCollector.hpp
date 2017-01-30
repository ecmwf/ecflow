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
class VItem;

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

    virtual bool add(VItem*, VItem*,Mode) = 0;
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
    TriggerListCollector(bool extended) :
        extended_(extended) {}

    ~TriggerListCollector();
    bool add(VItem*, VItem*,Mode);
    bool scanParents() { return extended_; }
    bool scanKids() { return extended_; }
    void setDependency(bool);
    void clear();

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

    bool add(VItem*, VItem*,Mode);

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

    bool add(VItem*, VItem*,Mode);

private:
  VItem* parent_;
  TriggerCollector* collector_;
};

class TriggeredCollector : public TriggerListCollector
{
public:
    TriggeredCollector(VNode* n) :
        TriggerListCollector(false), node_(n) {}
    bool add(VItem*, VItem*,Mode);

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


#endif // TRIGGERCOLLECTOR_HPP

