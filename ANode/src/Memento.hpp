#ifndef MEMENTO_HPP_
#define MEMENTO_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #41 $ 
//
// Copyright 2009-2017 ECMWF.
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

//#define DEBUG_MEMENTO 1

class Memento : private boost::noncopyable {
public:
   virtual ~Memento();
private:
   /// Applies the mementos to the client side defs. Can raise std::runtime_error
   virtual void do_incremental_node_sync(Node*,std::vector<ecf::Aspect::Type>& aspects,bool) const {}
   virtual void do_incremental_task_sync(Task*,std::vector<ecf::Aspect::Type>& aspects,bool) const {}
   virtual void do_incremental_alias_sync(Alias*,std::vector<ecf::Aspect::Type>& aspects,bool) const {}
   virtual void do_incremental_suite_sync(Suite*,std::vector<ecf::Aspect::Type>& aspects,bool) const {}
   virtual void do_incremental_family_sync(Family*,std::vector<ecf::Aspect::Type>& aspects,bool) const {}
   virtual void do_incremental_defs_sync(Defs*,std::vector<ecf::Aspect::Type>& aspects,bool) const {}
   friend class CompoundMemento;

private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {}
};


// Used for storing all the memento's associated with a single node
// This allow us to make only *ONE* call to find the node.
// The mementos are then applied to this single node.
class CompoundMemento  {
public:
   explicit CompoundMemento(const std::string& absNodePath)
   : clear_attributes_(false),absNodePath_(absNodePath) {}

   CompoundMemento()= default; // for serialization

   void incremental_sync(defs_ptr client_def) const;
   void add(memento_ptr m) { vec_.push_back(m); }
   void clear_attributes() { clear_attributes_ = true;}

   const std::string& abs_node_path() const { return absNodePath_;}

private:

   bool clear_attributes_{false};
   std::string absNodePath_;
   std::vector<memento_ptr> vec_;
   mutable std::vector<ecf::Aspect::Type> aspects_; // not persisted only used on client side

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      CEREAL_OPTIONAL_NVP(ar, clear_attributes_, [this](){return clear_attributes_; });  // conditionally save
      ar(CEREAL_NVP(absNodePath_),
         CEREAL_NVP(vec_));
   }
};


class StateMemento : public Memento {
public:
   explicit StateMemento(NState::State state) : state_(state) {}
   StateMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f)    const override { n->set_memento(this,aspects,f);}
   void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { defs->set_memento(this,aspects,f);}

   NState::State state_{NState::UNKNOWN};
   friend class Node;
   friend class Defs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(state_));
   }
};

class OrderMemento : public Memento {
public:
   explicit OrderMemento(const std::vector<std::string>& order) : order_(order) {}
   OrderMemento() = default;
private:
   void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { defs->set_memento(this,aspects,f);}
   void do_incremental_suite_sync(Suite* s,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { s->set_memento(this,aspects,f);}
   void do_incremental_family_sync(Family* f,std::vector<ecf::Aspect::Type>& aspects,bool ff) const override { f->set_memento(this,aspects,ff);}
   void do_incremental_task_sync(Task* t,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { t->set_memento(this,aspects,f);}

   std::vector<std::string> order_;
   friend class NodeContainer;
   friend class Task;
   friend class Defs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(order_));
   }
};

class ChildrenMemento : public Memento {
public:
   explicit ChildrenMemento(const std::vector<node_ptr>& children) : children_(children) {}
   ChildrenMemento() = default;
private:
   void do_incremental_suite_sync(Suite* s,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { s->set_memento(this,aspects,f);}
   void do_incremental_family_sync(Family* f,std::vector<ecf::Aspect::Type>& aspects,bool ff) const override { f->set_memento(this,aspects,ff);}

   std::vector<node_ptr> children_;
   friend class NodeContainer;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(children_));
   }
};

class AliasChildrenMemento : public Memento {
public:
   explicit AliasChildrenMemento(const std::vector<alias_ptr>& children) : children_(children) {}
   AliasChildrenMemento() = default;
private:
   void do_incremental_task_sync(Task* t,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { t->set_memento(this,aspects,f);}

   std::vector<alias_ptr> children_;
   friend class Task;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(children_));
   }
};

class AliasNumberMemento : public Memento {
public:
   explicit AliasNumberMemento(unsigned int alias_no ) : alias_no_(alias_no) {}
   AliasNumberMemento()= default;
private:
   void do_incremental_task_sync(Task* t,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { t->set_memento(this,aspects,f);}

   unsigned int alias_no_{0};
   friend class Task;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(alias_no_));
   }
};


class SuspendedMemento : public Memento {
public:
   explicit SuspendedMemento(bool suspended) : suspended_(suspended) {}
   SuspendedMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f)    const override { n->set_memento(this,aspects,f);}

   bool suspended_{false};
   friend class Node;
   friend class Defs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(suspended_));
   }
};

class ServerStateMemento : public Memento {
public:
   explicit ServerStateMemento(SState::State s) : state_(s) {}
   ServerStateMemento()= default;
private:
   void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { defs->set_memento(this,aspects,f);}

   SState::State state_{SState::HALTED};
   friend class Defs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(state_));
   }
};

class ServerVariableMemento : public Memento {
public:
   explicit ServerVariableMemento(const std::vector<Variable>& vec) : serverEnv_(vec) {}
   ServerVariableMemento() = default;
private:
   void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { defs->set_memento(this,aspects,f);}

   std::vector<Variable>  serverEnv_;
   friend class Defs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(serverEnv_));
   }
};

class NodeDefStatusDeltaMemento : public Memento {
public:
   explicit NodeDefStatusDeltaMemento(DState::State state) : state_(state) {}
   NodeDefStatusDeltaMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   DState::State state_{DState::UNKNOWN};
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(state_));
   }
};

class NodeEventMemento : public Memento {
public:
   explicit NodeEventMemento( const Event& e) : event_(e) {}
   NodeEventMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Event event_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(event_));
   }
};

class NodeMeterMemento : public Memento {
public:
   explicit NodeMeterMemento(const Meter& e) : meter_(e) {}
   NodeMeterMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Meter meter_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(meter_));
   }
};


class NodeLabelMemento : public Memento {
public:
   explicit NodeLabelMemento( const Label& e) : label_(e) {}
   NodeLabelMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Label label_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(label_));
   }
};

class NodeQueueMemento : public Memento {
public:
   NodeQueueMemento(const QueueAttr& e) : queue_(e) {}
   NodeQueueMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   QueueAttr queue_;
   friend class Node;
   friend class MiscAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(queue_));
   }
};

class NodeGenericMemento : public Memento {
public:
   NodeGenericMemento(const GenericAttr& e) : generic_(e) {}
   NodeGenericMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   GenericAttr generic_;
   friend class Node;
   friend class MiscAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(generic_));
   }
};

class NodeQueueIndexMemento : public Memento {
public:
   NodeQueueIndexMemento(const std::string& name, int index,const std::vector<NState::State>& state_vec)
    : index_(index), name_(name),state_vec_(state_vec) {}
   NodeQueueIndexMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   int index_{0};
   std::string name_;
   std::vector<NState::State> state_vec_;
   friend class Node;
   friend class MiscAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(index_),
         CEREAL_NVP(name_),
         CEREAL_NVP(state_vec_));
   }
};


class NodeTriggerMemento : public Memento {
public:
   explicit NodeTriggerMemento(const Expression& e) : exp_(e) {}
   NodeTriggerMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Expression exp_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(exp_));
   }
};

class NodeCompleteMemento : public Memento {
public:
   explicit NodeCompleteMemento(const Expression& e) : exp_(e) {}
   NodeCompleteMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Expression exp_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(exp_));
   }
};

class NodeRepeatMemento : public Memento {
public:
   explicit NodeRepeatMemento( const Repeat& e ) : repeat_(e) {}
   NodeRepeatMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Repeat repeat_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(repeat_));
   }
};

class NodeRepeatIndexMemento : public Memento {
public:
   NodeRepeatIndexMemento( const Repeat& e ) : index_or_value_(e.index_or_value()) {}
   NodeRepeatIndexMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   long index_or_value_{0};
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(index_or_value_));
   }
};


class NodeLimitMemento : public Memento {
public:
   explicit NodeLimitMemento( const Limit& e) : limit_( e ) {}
   NodeLimitMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Limit limit_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(limit_));
   }
};

class NodeInLimitMemento : public Memento {
public:
   explicit NodeInLimitMemento( const InLimit& e) : inlimit_( e ) {}
   NodeInLimitMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   InLimit inlimit_;
   friend class Node;
   friend class InLimitMgr;

   friend class cereal::access;
   template<class Archive>
   void serialize( Archive & ar, std::uint32_t const version  ) {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(inlimit_));
   }
};

class NodeVariableMemento : public Memento {
public:
   explicit NodeVariableMemento( const Variable& e) : var_(e) {}
   NodeVariableMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   Variable var_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(var_));
   }
};

class NodeLateMemento : public Memento {
public:
   explicit NodeLateMemento( const ecf::LateAttr& e) : late_(e) {}
   NodeLateMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   ecf::LateAttr late_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(late_));
   }
};

class FlagMemento : public Memento {
public:
   explicit FlagMemento( const ecf::Flag& e) : flag_(e) {}
   FlagMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}
   void do_incremental_defs_sync(Defs* defs,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { defs->set_memento(this,aspects,f);}

   ecf::Flag flag_;
   friend class Node;
   friend class Defs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(flag_));
   }
};

class NodeTodayMemento : public Memento {
public:
   explicit NodeTodayMemento( const ecf::TodayAttr& attr) : attr_(attr) {}
   NodeTodayMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   ecf::TodayAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
   }
};

class NodeTimeMemento : public Memento {
public:
   explicit NodeTimeMemento( const ecf::TimeAttr& attr) : attr_(attr) {}
   NodeTimeMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   ecf::TimeAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
   }
};

class NodeDayMemento : public Memento {
public:
   explicit NodeDayMemento( const DayAttr& attr) : attr_(attr) {}
   NodeDayMemento()= default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   DayAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
   }
};

class NodeCronMemento : public Memento {
public:
   explicit NodeCronMemento( const ecf::CronAttr& attr) : attr_(attr) {}
   NodeCronMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   ecf::CronAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
   }
};

class NodeDateMemento : public Memento {
public:
   explicit NodeDateMemento( const DateAttr& attr) : attr_(attr) {}
   NodeDateMemento()  = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   DateAttr attr_;
   friend class Node;
   friend class TimeDepAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
   }
};

class NodeZombieMemento : public Memento {
public:
   explicit NodeZombieMemento(const ZombieAttr& attr) : attr_(attr) {}
   NodeZombieMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   ZombieAttr attr_;
   friend class Node;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
   }
};


class NodeVerifyMemento : public Memento {
public:
   explicit NodeVerifyMemento(const std::vector<VerifyAttr>& attr) : verifys_(attr) {}
   NodeVerifyMemento() = default;
private:
   void do_incremental_node_sync(Node* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   std::vector<VerifyAttr> verifys_;
   friend class Node;
   friend class MiscAttrs;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(verifys_));
   }
};

class SubmittableMemento : public Memento {
public:
   SubmittableMemento(	const std::string& jobsPassword,
            const std::string& process_or_remote_id,
            const std::string& abortedReason,
            int tryNo
   ) :
      paswd_( jobsPassword ),
      rid_( process_or_remote_id ),
      abr_( abortedReason ),
      tryNo_( tryNo ) {}
   SubmittableMemento()= default;
private:
   void do_incremental_task_sync(Task* n,std::vector<ecf::Aspect::Type>& aspects,bool f)   const override { n->set_memento(this,aspects,f);}
   void do_incremental_alias_sync(Alias* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   std::string  paswd_;
   std::string  rid_;
   std::string  abr_;
   int          tryNo_{0};
   friend class Submittable;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
         CEREAL_NVP(paswd_),
         CEREAL_NVP(rid_),
         CEREAL_NVP(abr_),
         CEREAL_NVP(tryNo_));
   }
};

class SuiteClockMemento : public Memento {
public:
   explicit SuiteClockMemento( const ClockAttr& c ) :   clockAttr_(c) {}
   SuiteClockMemento() = default;
private:
   void do_incremental_suite_sync(Suite* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   ClockAttr  clockAttr_;
   friend class Suite;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(clockAttr_));
   }
};

class SuiteBeginDeltaMemento : public Memento {
public:
   explicit SuiteBeginDeltaMemento(bool begun) : begun_(begun) {}
   SuiteBeginDeltaMemento()= default;
private:
   void do_incremental_suite_sync(Suite* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   bool begun_{false};
   friend class Suite;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(begun_));
   }
};

class SuiteCalendarMemento : public Memento {
public:
   explicit SuiteCalendarMemento(const ecf::Calendar& cal) : cal_(cal) {}
   SuiteCalendarMemento() = default;
private:
   void do_incremental_suite_sync(Suite* n,std::vector<ecf::Aspect::Type>& aspects,bool f) const override { n->set_memento(this,aspects,f);}

   ecf::Calendar  cal_;          // *Only* persisted since used by the why() on client side
   friend class Suite;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(cal_));
   }
};

#endif
