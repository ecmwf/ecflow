#ifndef TASK_HPP_
#define TASK_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #108 $ 
//
// Copyright 2009-2019 ECMWF.
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
   explicit Task( const std::string& name ) : Submittable(name) {}
   Task()= default;
   Task(const Task& rhs);
   Task& operator=(const Task&);
   node_ptr clone() const override;
	~Task() override;

   bool check_defaults() const override;

	static task_ptr create(const std::string& name);

	void print(std::string&) const override;
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
   void immediateChildren(std::vector<node_ptr>&) const override;

   /// Overidden from Submittable
   const std::string& script_extension() const override;

   node_ptr find_node_up_the_tree(const std::string& name) const override;

   /// Added for consistency, really used to find relative nodes, aliases should never, be in referenced nodes
   node_ptr find_relative_node(const std::vector<std::string>& pathToNode) override { return node_ptr();}

	/// Overridden to reset the try number
	/// The tasks job can be invoked multiple times. For each invocation we want to preserve
	/// the output. The try number is used in SMSJOB/SMSJOBOUT to preserve the output when
	/// there are multiple runs.  re-queue/begin() resets the try Number
   void reset() override;
   void begin() override;
	void requeue(Requeue_args&) override;

   Suite* suite() const override { return parent()->suite(); }
   Defs* defs() const override { return (parent()) ? parent()->defs() : nullptr;} // exposed to python hence check for NULL first
	Task* isTask() const override   { return const_cast<Task*>(this);}
   Submittable* isSubmittable() const override { return const_cast<Task*>(this); }

	void accept(ecf::NodeTreeVisitor&) override;
	void acceptVisitTraversor(ecf::NodeTreeVisitor& v) override;

   void getAllNodes(std::vector<Node*>&) const override;
   void getAllTasks(std::vector<Task*>&) const override;
   void getAllSubmittables(std::vector<Submittable*>&) const override;
   void get_all_active_submittables(std::vector<Submittable*>&) const override;
   void get_all_tasks(std::vector<task_ptr>&) const override;
   void get_all_nodes(std::vector<node_ptr>&) const override;
   void get_all_aliases(std::vector<alias_ptr>&) const override;

	const std::string& debugType() const override;

	/// submits the jobs of the dependencies resolve
	bool resolveDependencies(JobsParam& jobsParam) override;

  	node_ptr removeChild( Node* child) override;
 	bool addChild( const node_ptr& child,size_t position = std::numeric_limits<std::size_t>::max()) override;
 	bool isAddChildOk( Node* child, std::string& errorMsg) const override;

   void order(Node* immediateChild, NOrder::Order) override;
   void generate_scripts( const std::map<std::string,std::string>& override) const override;

   bool checkInvariants(std::string& errorMsg) const override;

   void collateChanges(DefsDelta&) const override;
   void set_memento(const OrderMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const AliasChildrenMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const AliasNumberMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const SubmittableMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { Submittable::set_memento(m,aspects,f); }

   void read_state(const std::string& line,const std::vector<std::string>& lineTokens) override;
private:
   void copy(const Task&);
   size_t child_position(const Node*) const override;
   std::string write_state() const override;

private:
   /// For use by python interface,
   std::vector<alias_ptr>::const_iterator alias_begin() const { return aliases_.begin();}
   std::vector<alias_ptr>::const_iterator alias_end() const   { return aliases_.end();}
   friend void export_Task();

private:
	// Overridden from Node to increment/decrement limits
	void handleStateChange() override;
   bool doDeleteChild(Node* child) override;

   // Overridden to locate alias's
   node_ptr findImmediateChild(const std::string& name, size_t& child_pos) const override;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );

private:
   unsigned int order_state_change_no_{0};     // no need to persist
   unsigned int add_remove_state_change_no_{0};// no need to persist

   unsigned int alias_change_no_{0};           // no need to persist, for alias number only
 	unsigned int alias_no_{0};
 	std::vector<alias_ptr> aliases_;
};

std::ostream& operator<<(std::ostream& os, const Task&);

#endif
