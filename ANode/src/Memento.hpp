#ifndef MEMENTO_HPP_
#define MEMENTO_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #41 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : class Memento
//               Class's derived from Memento are stored on DefsDelta.
//               DefsDelta is transferred from Server to Client during a sync
//               See SSyncCmd.hpp
//
// Are created in the server, but used by the client to sync.
//
// Used to capture incremental change of state, to node, and node attributes
// Serve as a base class of all memento's
// Later on the client side, the changes can be applied. via incremental_sync()
// The are several kind of changes that we can capture:
// 	a/ simple state changes,
// 	b/ Change in attribute structure
// 	c/ Deletion of attribute
// 	d/ Addition of an attribute
// 	e/ Add/delete of a Family/task
//    f/ Add/Delete of suite
//
// The main emphasis here is to capture a,b,c,d,e. This is easily handled by state_change_no.
// option f/ is handled via a full update and hence does not use mementos
//
// ISSUES: AIX has issues with TOC(table of contents) overflow. This is heavily
// influenced by the number of global symbols. Unfortunately each boost serializiable
// type, greatly increases the number of globals.
// Hence we need to ensure we use the minimum number of serializable types.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Alias.hpp"

#include "boost_archive.hpp" // collates boost archive includes
#include <boost/serialization/export.hpp>
#include <boost/serialization/deque.hpp>          // no need to include <deque>

//#define DEBUG_MEMENTO 1

class Memento : private boost::noncopyable {
public:
   virtual ~Memento();
private:
   /// Applies the mementos to the client side defs. Can raise std::runtime_error
   virtual void do_incremental_node_sync(Node*,std::vector<ecf::Aspect::Type>& aspects) const {}
   virtual void do_incremental_task_sync(Task*,std::vector<ecf::Aspect::Type>& aspects) const {}
   virtual void do_incremental_alias_sync(Alias*,std::vector<ecf::Aspect::Type>& aspects) const {}
   virtual void do_incremental_suite_sync(Suite*,std::vector<ecf::Aspect::Type>& aspects) const {}
   virtual void do_incremental_family_sync(Family*,std::vector<ecf::Aspect::Type>& aspects) const {}
   virtual void do_incremental_defs_sync(Defs*,std::vector<ecf::Aspect::Type>& aspects) const {}
   friend class CompoundMemento;

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {}
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Memento)


// Used for storing all the memento's associated with a single node
// This allow us to make only *ONE* call to find the node.
// The mementos are then applied to this single node.
class CompoundMemento  {
public:
   CompoundMemento(const std::string& absNodePath)
   : clear_attributes_(false),absNodePath_(absNodePath) {}

   CompoundMemento() : clear_attributes_(false) {} // for serialization

   void incremental_sync(defs_ptr client_def, std::vector<std::string>& changed_nodes) const;
   void add(memento_ptr m) { vec_.push_back(m); }
   void clear_attributes() { clear_attributes_ = true;}

private:

   bool clear_attributes_;
   std::string absNodePath_;
   std::vector<memento_ptr> vec_;
   mutable std::vector<ecf::Aspect::Type> aspects_; // not persisted only used on client side

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & clear_attributes_;
      ar & absNodePath_;
      ar & vec_;
   }
};


class StateMemento : public Memento {
public:
   StateMemento(NState::State state) : state_(state) {}
   StateMemento() : state_(NState::UNKNOWN) {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects)    const { n->set_memento(this,aspects);}
   virtual void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects) const { defs->set_memento(this,aspects);}

   NState::State state_;
   friend class Node;
   friend class Defs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & state_;
   }
};

class OrderMemento : public Memento {
public:
   OrderMemento(const std::vector<std::string>& order) : order_(order) {}
   OrderMemento() {}
private:
   virtual void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects) const { defs->set_memento(this,aspects);}
   virtual void do_incremental_suite_sync(Suite* s,std::vector<ecf::Aspect::Type>& aspects) const { s->set_memento(this,aspects);}
   virtual void do_incremental_family_sync(Family* f,std::vector<ecf::Aspect::Type>& aspects) const { f->set_memento(this,aspects);}
   virtual void do_incremental_task_sync(Task* t,std::vector<ecf::Aspect::Type>& aspects) const { t->set_memento(this,aspects);}

   std::vector<std::string> order_;
   friend class NodeContainer;
   friend class Task;
   friend class Defs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & order_;
   }
};

class ChildrenMemento : public Memento {
public:
   ChildrenMemento(const std::vector<node_ptr>& children) : children_(children) {}
   ChildrenMemento() {}
private:
   virtual void do_incremental_suite_sync(Suite* s,std::vector<ecf::Aspect::Type>& aspects) const { s->set_memento(this,aspects);}
   virtual void do_incremental_family_sync(Family* f,std::vector<ecf::Aspect::Type>& aspects) const { f->set_memento(this,aspects);}

   std::vector<node_ptr> children_;
   friend class NodeContainer;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {

      ar.register_type(static_cast<Task *>(NULL));
      ar.register_type(static_cast<Family *>(NULL));

      ar & boost::serialization::base_object<Memento>(*this);
      ar & children_;
   }
};

class AliasChildrenMemento : public Memento {
public:
   AliasChildrenMemento(const std::vector<alias_ptr>& children) : children_(children) {}
   AliasChildrenMemento() {}
private:
   virtual void do_incremental_task_sync(Task* t,std::vector<ecf::Aspect::Type>& aspects) const { t->set_memento(this,aspects);}

   std::vector<alias_ptr> children_;
   friend class Task;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {

      ar.register_type(static_cast<Alias *>(NULL));

      ar & boost::serialization::base_object<Memento>(*this);
      ar & children_;
   }
};

class AliasNumberMemento : public Memento {
public:
   AliasNumberMemento(unsigned int alias_no ) : alias_no_(alias_no) {}
   AliasNumberMemento() : alias_no_(0) {}
private:
   virtual void do_incremental_task_sync(Task* t,std::vector<ecf::Aspect::Type>& aspects) const { t->set_memento(this,aspects);}

   unsigned int alias_no_;
   friend class Task;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & alias_no_;
   }
};


class SuspendedMemento : public Memento {
public:
   SuspendedMemento(bool suspended) : suspended_(suspended) {}
   SuspendedMemento() : suspended_(false) {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects)    const { n->set_memento(this,aspects);}

   bool suspended_;
   friend class Node;
   friend class Defs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & suspended_;
   }
};

class ServerStateMemento : public Memento {
public:
   ServerStateMemento(SState::State s) : state_(s) {}
   ServerStateMemento() : state_(SState::HALTED) {}
private:
   virtual void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects) const { defs->set_memento(this,aspects);}

   SState::State state_;
   friend class Defs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & state_;
   }
};

class ServerVariableMemento : public Memento {
public:
   ServerVariableMemento(const std::vector<Variable>& vec) : serverEnv_(vec) {}
   ServerVariableMemento() {}
private:
   virtual void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects) const { defs->set_memento(this,aspects);}

   std::vector<Variable>  serverEnv_;
   friend class Defs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & serverEnv_;
   }
};

class NodeDefStatusDeltaMemento : public Memento {
public:
   NodeDefStatusDeltaMemento(DState::State state) : state_(state) {}
   NodeDefStatusDeltaMemento() : state_(DState::UNKNOWN) {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   DState::State state_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & state_;
   }
};

class NodeEventMemento : public Memento {
public:
   NodeEventMemento( const Event& e) : event_(e) {}
   NodeEventMemento(){}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Event event_;
   friend class Node;
   friend class ChildAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & event_;
   }
};

class NodeMeterMemento : public Memento {
public:
   NodeMeterMemento(const Meter& e) : meter_(e) {}
   NodeMeterMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Meter meter_;
   friend class Node;
   friend class ChildAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & meter_;
   }
};

class NodeLabelMemento : public Memento {
public:
   NodeLabelMemento( const Label& e) : label_(e) {}
   NodeLabelMemento(){}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Label label_;
   friend class Node;
   friend class ChildAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & label_;
   }
};


class NodeTriggerMemento : public Memento {
public:
   NodeTriggerMemento(const Expression& e) : exp_(e) {}
   NodeTriggerMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Expression exp_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & exp_;
   }
};

class NodeCompleteMemento : public Memento {
public:
   NodeCompleteMemento(const Expression& e) : exp_(e) {}
   NodeCompleteMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Expression exp_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & exp_;
   }
};

class NodeRepeatMemento : public Memento {
public:
   NodeRepeatMemento( const Repeat& e ) : repeat_(e) {}
   NodeRepeatMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Repeat repeat_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & repeat_;
   }
};

class NodeLimitMemento : public Memento {
public:
   NodeLimitMemento( const Limit& e) : limit_( e ) {}
   NodeLimitMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Limit limit_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & limit_;
   }
};

class NodeInLimitMemento : public Memento {
public:
   NodeInLimitMemento( const InLimit& e) : inlimit_( e ) {}
   NodeInLimitMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   InLimit inlimit_;
   friend class Node;
   friend class InLimitMgr;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & inlimit_;
   }
};

class NodeVariableMemento : public Memento {
public:
   NodeVariableMemento( const Variable& e) : var_(e) {}
   NodeVariableMemento(){}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   Variable var_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & var_;
   }
};

class NodeLateMemento : public Memento {
public:
   NodeLateMemento( const ecf::LateAttr& e) : late_(e) {}
   NodeLateMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   ecf::LateAttr late_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & late_;
   }
};

class FlagMemento : public Memento {
public:
   FlagMemento( const ecf::Flag& e) : flag_(e) {}
   FlagMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}
   virtual void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects) const { defs->set_memento(this,aspects);}

   ecf::Flag flag_;
   friend class Node;
   friend class Defs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & flag_;
   }
};

class NodeTodayMemento : public Memento {
public:
   NodeTodayMemento( const ecf::TodayAttr& attr) : attr_(attr) {}
   NodeTodayMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   ecf::TodayAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & attr_;
   }
};

class NodeTimeMemento : public Memento {
public:
   NodeTimeMemento( const ecf::TimeAttr& attr) : attr_(attr) {}
   NodeTimeMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   ecf::TimeAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & attr_;
   }
};

class NodeDayMemento : public Memento {
public:
   NodeDayMemento( const DayAttr& attr) : attr_(attr) {}
   NodeDayMemento(){}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   DayAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & attr_;
   }
};

class NodeCronMemento : public Memento {
public:
   NodeCronMemento( const ecf::CronAttr& attr) : attr_(attr) {}
   NodeCronMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   ecf::CronAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & attr_;
   }
};

class NodeDateMemento : public Memento {
public:
   NodeDateMemento( const DateAttr& attr) : attr_(attr) {}
   NodeDateMemento()  {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   DateAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & attr_;
   }
};

class NodeZombieMemento : public Memento {
public:
   NodeZombieMemento(const ZombieAttr& attr) : attr_(attr) {}
   NodeZombieMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   ZombieAttr attr_;
   friend class Node;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & attr_;
   }
};


class NodeVerifyMemento : public Memento {
public:
   NodeVerifyMemento(const std::vector<VerifyAttr>& attr) : verifys_(attr) {}
   NodeVerifyMemento() {}
private:
   virtual void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   std::vector<VerifyAttr> verifys_;
   friend class Node;
   friend class MiscAttrs;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & verifys_;
   }
};

class SubmittableMemento : public Memento {
public:
   SubmittableMemento(	const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            const std::string& abortedReason,
            int tryNo
   ) :
      jobsPassword_( jobsPassword ),
      process_or_remote_id_( process_or_remote_id ),
      abortedReason_( abortedReason ),
      tryNo_( tryNo ) {}
   SubmittableMemento() : tryNo_(0) {}
private:
   virtual void do_incremental_task_sync(Task* n,std::vector<ecf::Aspect::Type>& aspects)   const { n->set_memento(this,aspects);}
   virtual void do_incremental_alias_sync(Alias* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   std::string  jobsPassword_;
   std::string  process_or_remote_id_;
   std::string  abortedReason_;
   int          tryNo_;
   friend class Submittable;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & jobsPassword_;
      ar & process_or_remote_id_;
      ar & abortedReason_;
      ar & tryNo_;
   }
};

class SuiteClockMemento : public Memento {
public:
   SuiteClockMemento( const ClockAttr& c ) :   clockAttr_(c) {}
   SuiteClockMemento() {}
private:
   virtual void do_incremental_suite_sync(Suite* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   ClockAttr  clockAttr_;
   friend class Suite;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & clockAttr_;
   }
};

class SuiteBeginDeltaMemento : public Memento {
public:
   SuiteBeginDeltaMemento(bool begun) : begun_(begun) {}
   SuiteBeginDeltaMemento() : begun_(false) {}
private:
   virtual void do_incremental_suite_sync(Suite* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   bool begun_;
   friend class Suite;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & begun_;
   }
};

class SuiteCalendarMemento : public Memento {
public:
   SuiteCalendarMemento(const ecf::Calendar& cal) : calendar_(cal) {}
   SuiteCalendarMemento() {}
private:
   virtual void do_incremental_suite_sync(Suite* n,std::vector<ecf::Aspect::Type>& aspects) const { n->set_memento(this,aspects);}

   ecf::Calendar  calendar_;          // *Only* persisted since used by the why() on client side
   friend class Suite;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object<Memento>(*this);
      ar & calendar_;
   }
};


BOOST_CLASS_EXPORT_KEY(StateMemento);
BOOST_CLASS_EXPORT_KEY(NodeDefStatusDeltaMemento);
BOOST_CLASS_EXPORT_KEY(SuspendedMemento);
BOOST_CLASS_EXPORT_KEY(ServerStateMemento);
BOOST_CLASS_EXPORT_KEY(ServerVariableMemento);
BOOST_CLASS_EXPORT_KEY(NodeEventMemento);
BOOST_CLASS_EXPORT_KEY(NodeMeterMemento);
BOOST_CLASS_EXPORT_KEY(NodeLabelMemento);
BOOST_CLASS_EXPORT_KEY(NodeTriggerMemento);
BOOST_CLASS_EXPORT_KEY(NodeCompleteMemento);
BOOST_CLASS_EXPORT_KEY(NodeRepeatMemento);
BOOST_CLASS_EXPORT_KEY(NodeLimitMemento);
BOOST_CLASS_EXPORT_KEY(NodeInLimitMemento);
BOOST_CLASS_EXPORT_KEY(NodeVariableMemento);
BOOST_CLASS_EXPORT_KEY(NodeLateMemento);
BOOST_CLASS_EXPORT_KEY(NodeTodayMemento);
BOOST_CLASS_EXPORT_KEY(NodeTimeMemento);
BOOST_CLASS_EXPORT_KEY(NodeDayMemento);
BOOST_CLASS_EXPORT_KEY(NodeCronMemento);
BOOST_CLASS_EXPORT_KEY(NodeDateMemento);
BOOST_CLASS_EXPORT_KEY(NodeZombieMemento);
BOOST_CLASS_EXPORT_KEY(FlagMemento);
BOOST_CLASS_EXPORT_KEY(NodeVerifyMemento);
BOOST_CLASS_EXPORT_KEY(SubmittableMemento);
BOOST_CLASS_EXPORT_KEY(SuiteClockMemento);
BOOST_CLASS_EXPORT_KEY(SuiteBeginDeltaMemento);
BOOST_CLASS_EXPORT_KEY(SuiteCalendarMemento);
BOOST_CLASS_EXPORT_KEY(OrderMemento);
BOOST_CLASS_EXPORT_KEY(ChildrenMemento);
BOOST_CLASS_EXPORT_KEY(AliasChildrenMemento);
BOOST_CLASS_EXPORT_KEY(AliasNumberMemento);

#endif
