#ifndef NODE_FWD_HPP_
#define NODE_FWD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #44 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <memory>
#include <vector>
#include <string>
#include <map>

class AbstractObserver;
class Suite;
class Family;
class Task;
class Alias;
class Submittable;
class Node;
class Defs;
class Repeat;
class Expression;
class Memento;
class CompoundMemento;
class ClockAttr;
class JobCreationCtrl;
class Event;
class Meter;
class Repeat;
class Variable;

namespace ecf { class LateAttr; class AutoCancelAttr; class AutoArchiveAttr; class AutoRestoreAttr; } // forward declare class

typedef std::shared_ptr<Memento> memento_ptr;
typedef std::shared_ptr<CompoundMemento> compound_memento_ptr;
typedef std::shared_ptr<ClockAttr> clock_ptr;

typedef std::shared_ptr<JobCreationCtrl> job_creation_ctrl_ptr;
typedef std::shared_ptr<Node>   node_ptr;
typedef std::shared_ptr<Task>   task_ptr;
typedef std::shared_ptr<Alias>  alias_ptr;
typedef std::shared_ptr<Submittable>  submittable_ptr;
typedef std::shared_ptr<Family> family_ptr;
typedef std::shared_ptr<Suite>  suite_ptr;
typedef std::shared_ptr<Defs>   defs_ptr;

typedef std::weak_ptr<Defs>  weak_defs_ptr;
typedef std::weak_ptr<Suite> weak_suite_ptr;
typedef std::weak_ptr<Task>  weak_task_ptr;
typedef std::weak_ptr<Alias> weak_alias_ptr;
typedef std::weak_ptr<Submittable> weak_submittable_ptr;
typedef std::weak_ptr<Node>  weak_node_ptr;

typedef std::map<std::string,std::string> NameValueMap;

typedef std::vector< std::pair< std::string,std::string> > NameValueVec;


class NodeContainer;
class DefsDelta;
class JobsParam;
class JobCreationCtrl;
class AstTop;

class StateMemento;
class NodeDefStatusDeltaMemento;
class SuspendedMemento;
class ServerStateMemento;
class ServerVariableMemento;
class NodeEventMemento;
class NodeMeterMemento;
class NodeLabelMemento;
class NodeTriggerMemento;
class NodeCompleteMemento;
class NodeRepeatMemento;
class NodeRepeatIndexMemento;
class NodeLimitMemento;
class NodeInLimitMemento;
class NodeVariableMemento;
class NodeLateMemento;
class NodeTodayMemento;
class NodeTimeMemento;
class NodeDayMemento;
class NodeCronMemento;
class NodeDateMemento;
class NodeZombieMemento;
class NodeVerifyMemento;
class FlagMemento;
class SubmittableMemento;
class SuiteClockMemento;
class SuiteBeginDeltaMemento;
class SuiteCalendarMemento;
class OrderMemento;
class ChildrenMemento;
class AliasChildrenMemento;
class AliasNumberMemento;
class NodeQueueMemento;
class NodeGenericMemento;
class NodeQueueIndexMemento;

#endif
