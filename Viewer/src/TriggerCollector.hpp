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

class VItem;
class VNode;
class TriggerListItem;

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

    virtual void add(VItem*, VItem*,Mode,VItem*) = 0;
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

    void add(VItem*,VItem*,Mode,VItem*);
    bool scanParents() { return extended_; }
    bool scanKids() { return extended_; }

    QString text();


protected:
    //panel& p_;
    FILE* file_;
    std::string title_;
    bool extended_;
    //std::vector<VItem*> items_;
    std::vector<TriggerListItem*> items_;
};

class TriggerChildCollector : public TriggerCollector
{
public:
    TriggerChildCollector(VNode* n,VItem* child,TriggerCollector* collector) :
        node_(n), child_(child), collector_(collector) {}
    void add(VItem*, VItem*,Mode,VItem*);

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
    void add(VItem*, VItem*,Mode,VItem*);

private:
  VItem* parent_;
  TriggerCollector* collector_;
};

#if 0
class TriggeredCollector : public TriggerListCollector
{
public:
    TriggeredCollector(VNode* n) :
        TriggerListCollector(NULL,"",false), node_(n) {}
    void add(VItem*, VItem*,Mode,VItem*);

private:
  VNode* node_;
};
#endif

class TriggeredCollector : public TriggerListCollector
{
public:
    TriggeredCollector(VNode* n) :
        TriggerListCollector(NULL,"",false), node_(n) {}
    void add(VItem*, VItem*,Mode,VItem*);

private:
  VNode* node_;
};

class TriggeredChildCollector : public TriggerCollector
{
public:
    TriggeredChildCollector(VNode* n,VItem* child,TriggerCollector* collector) :
        node_(n), child_(child), collector_(collector) {}
    void add(VItem*, VItem*,Mode,VItem*);

private:
  VItem* node_;
  VItem* child_;
  TriggerCollector* collector_;
};

class TriggeredParentCollector : public TriggerCollector
{
public:
    TriggeredParentCollector(VNode* n,VItem* parent,TriggerCollector* collector) :
        node_(n), parent_(parent), collector_(collector) {}
    void add(VItem*, VItem*,Mode,VItem*);

private:
  VItem* node_;
  VItem* parent_;
  TriggerCollector* collector_;
};

#endif // TRIGGERCOLLECTOR_HPP

