#ifndef TASK_HPP_
#define TASK_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #108 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include "Alias.hpp"

class Task : public Submittable {
public:
   explicit Task( const std::string& name )
   : Submittable(name),
     order_state_change_no_(0),
     add_remove_state_change_no_(0),
     alias_change_no_(0),
     alias_no_(0)
     {}

   Task()
   : order_state_change_no_(0),
     add_remove_state_change_no_(0),
     alias_change_no_(0),
     alias_no_(0)
     {}

   Task(const Task& rhs);
   Task& operator=(const Task&);

   node_ptr clone() const;

	virtual ~Task();

	static task_ptr create(const std::string& name);

   std::ostream& print(std::ostream&) const;
   bool operator==(const Task& rhs) const;

   /// Add an alias. The .usr is populated with contents of user_file_contents
   /// If create_directory is unset we create the alias with creating directory or .usr file
   alias_ptr add_alias(std::vector<std::string>& user_file_contents,const NameValueVec& user_variables,bool create_directory = true);

   /// Add alias without creating directory & user file
   alias_ptr add_alias_only();

   /// For Addition of alias via Defs Files
   alias_ptr add_alias(const std::string& name);

   /// Given a name find the alias.
   alias_ptr find_alias(const std::string& name) const;

   /// Reset alias number. Used in testing
   void reset_alias_number();

   /// return list of aliases held by this task
   const std::vector<alias_ptr>& aliases() { return aliases_; }
   virtual void immediateChildren(std::vector<node_ptr>&) const;

   /// Overidden from Submittable
   virtual const std::string& script_extension() const;

   virtual node_ptr find_node_up_the_tree(const std::string& name) const;

   /// Added for consistency, really used to find relative nodes, aliases should never, be in referenced nodes
   virtual node_ptr find_relative_node(const std::vector<std::string>& pathToNode) { return node_ptr();}

	/// Overridden to reset the try number
	/// The tasks job can be invoked multiple times. For each invocation we want to preserve
	/// the output. The try number is used in SMSJOB/SMSJOBOUT to preserve the output when
	/// there are multiple runs.  re-queue/begin() resets the try Number
   virtual void reset();
   virtual void begin();
	virtual void requeue(Requeue_args&);

   virtual Suite* suite() const { return parent()->suite(); }
   virtual Defs* defs() const { return (parent()) ? parent()->defs() : NULL;} // exposed to python hence check for NULL first
	virtual Task* isTask() const   { return const_cast<Task*>(this);}
   virtual Submittable* isSubmittable() const { return const_cast<Task*>(this); }

	virtual void accept(ecf::NodeTreeVisitor&);
	virtual void acceptVisitTraversor(ecf::NodeTreeVisitor& v);

   virtual void getAllNodes(std::vector<Node*>&) const;
   virtual void getAllTasks(std::vector<Task*>&) const;
   virtual void getAllSubmittables(std::vector<Submittable*>&) const;
   virtual void get_all_active_submittables(std::vector<Submittable*>&) const;
   virtual void get_all_tasks(std::vector<task_ptr>&) const;
   virtual void get_all_nodes(std::vector<node_ptr>&) const;
   virtual void get_all_aliases(std::vector<alias_ptr>&) const;

	virtual const std::string& debugType() const;

	/// submits the jobs of the dependencies resolve
	virtual bool resolveDependencies(JobsParam& jobsParam);

  	virtual node_ptr removeChild( Node* child);
 	virtual bool addChild( node_ptr child,size_t position = std::numeric_limits<std::size_t>::max());
 	virtual bool isAddChildOk( Node* child, std::string& errorMsg) const;

   virtual void order(Node* immediateChild, NOrder::Order);
   virtual void generate_scripts( const std::map<std::string,std::string>& override) const;

   virtual bool checkInvariants(std::string& errorMsg) const;

   virtual void collateChanges(DefsDelta&) const;
   void set_memento(const OrderMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const AliasChildrenMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const AliasNumberMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const SubmittableMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { Submittable::set_memento(m,aspects,f); }

   virtual void read_state(const std::string& line,const std::vector<std::string>& lineTokens);
private:
   void copy(const Task&);
   virtual size_t child_position(const Node*) const;
   virtual std::string write_state() const;

private:
   /// For use by python interface,
   std::vector<alias_ptr>::const_iterator alias_begin() const { return aliases_.begin();}
   std::vector<alias_ptr>::const_iterator alias_end() const   { return aliases_.end();}
   friend void export_Task();

private:
	// Overridden from Node to increment/decrement limits
	virtual void handleStateChange();
   virtual bool doDeleteChild(Node* child);

   // Overridden to locate alias's
   virtual node_ptr findImmediateChild(const std::string& name, size_t& child_pos) const;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(cereal::base_class<Submittable>(this));
      CEREAL_OPTIONAL_NVP(ar, alias_no_, [this](){return alias_no_ != 0; });    // conditionally save
      CEREAL_OPTIONAL_NVP(ar, aliases_ , [this](){return !aliases_.empty(); }); // conditionally save

      // Setup the alias parent pointers. Since they are not serialised
      // ********************************************************************
      // WE do not serialise the Alias parent pointer:
      // WHY: AliasChildrenMenento saves a vector of aliases, had we serialised
      //      the parent, it would also serialise the parent pointer(Task)
      //      i.e the ENTIRE task, and then Task parent, and so on, up the parent hierarchy.
      //      In our case it lead to unregistered class exception when trying to
      //      serialise the Task's parent.
      // SOLN: WE will not serialise the alias parent pointer. This will be left to
      //       Parent task.
      // ********************************************************************
      if (Archive::is_loading::value) {
         size_t vec_size = aliases_.size();
         for(size_t i = 0; i < vec_size; i++) {
            aliases_[i]->set_parent(this);
         }
      }
 	}

private:
   unsigned int order_state_change_no_;     // no need to persist
   unsigned int add_remove_state_change_no_;// no need to persist

   unsigned int alias_change_no_;           // no need to persist, for alias number only
 	unsigned int alias_no_;
 	std::vector<alias_ptr> aliases_;
};

std::ostream& operator<<(std::ostream& os, const Task&);

#endif
