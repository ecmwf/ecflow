/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_NodeFwd_HPP
#define ecflow_node_NodeFwd_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

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
class Zombie;
struct NodeStats;
class MiscAttrs;
class VerifyAttr;
class ZombieAttr;
class QueueAttr;
class GenericAttr;
class PartExpression;

namespace ecf {
class LateAttr;
class AutoCancelAttr;
class AutoArchiveAttr;
class AutoRestoreAttr;
} // namespace ecf

using memento_ptr          = std::shared_ptr<Memento>;
using compound_memento_ptr = std::shared_ptr<CompoundMemento>;
using clock_ptr            = std::shared_ptr<ClockAttr>;

using job_creation_ctrl_ptr = std::shared_ptr<JobCreationCtrl>;
using node_ptr              = std::shared_ptr<Node>;
using task_ptr              = std::shared_ptr<Task>;
using alias_ptr             = std::shared_ptr<Alias>;
using submittable_ptr       = std::shared_ptr<Submittable>;
using family_ptr            = std::shared_ptr<Family>;
using suite_ptr             = std::shared_ptr<Suite>;
using defs_ptr              = std::shared_ptr<Defs>;

using weak_defs_ptr        = std::weak_ptr<Defs>;
using weak_suite_ptr       = std::weak_ptr<Suite>;
using weak_task_ptr        = std::weak_ptr<Task>;
using weak_alias_ptr       = std::weak_ptr<Alias>;
using weak_submittable_ptr = std::weak_ptr<Submittable>;
using weak_node_ptr        = std::weak_ptr<Node>;

using NameValueMap = std::map<std::string, std::string>;
using NameValueVec = std::vector<std::pair<std::string, std::string>>;

class NodeContainer;
class DefsDelta;
class JobsParam;
class JobCreationCtrl;
class AstTop;
class Ast;

class StateMemento;
class NodeStateMemento;
class NodeDefStatusDeltaMemento;
class SuspendedMemento;
class ServerStateMemento;
class ServerVariableMemento;
class NodeEventMemento;
class NodeMeterMemento;
class NodeLabelMemento;
class NodeAvisoMemento;
class NodeMirrorMemento;
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

#endif /* ecflow_node_NodeFwd_HPP */
