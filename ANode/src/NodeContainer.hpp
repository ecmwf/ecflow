#ifndef NODE_CONTAINER_HPP_
#define NODE_CONTAINER_HPP_

//============================================================================
// Name        : NodeContainer.hpp
// Author      : Avi
// Revision    : $Revision: #88 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : A class that holds families and tasks
//============================================================================

#include <limits>
#include "Node.hpp"

class NodeContainer : public Node {
protected:
   NodeContainer& operator=(const NodeContainer&);
public:
   explicit NodeContainer( const std::string& name,bool check);
   NodeContainer(const NodeContainer& );
	NodeContainer();
	~NodeContainer() override;

   bool check_defaults() const override;

   void accept(ecf::NodeTreeVisitor&) override;
   void acceptVisitTraversor(ecf::NodeTreeVisitor& v) override;
   void reset() override;
	void begin() override;
	void requeue(Requeue_args&) override;
   void requeue_time_attrs() override;
	bool run(JobsParam& jobsParam, bool force) override;
	void kill(const std::string& zombie_pid = "") override;
	void status() override;
	bool top_down_why(std::vector<std::string>& theReasonWhy,bool html_tags = false) const override;
	void collateChanges(DefsDelta&) const override;
   void set_memento(const OrderMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   void set_memento(const ChildrenMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   void order(Node* immediateChild, NOrder::Order) override;

	bool hasAutoCancel() const override;
   void calendarChanged(const ecf::Calendar&,Node::Calendar_args&,const ecf::LateAttr* inherited_late) override;
 	bool resolveDependencies(JobsParam& ) override;
   bool has_time_based_attributes() const override;
 	bool check(std::string& errorMsg, std::string& warningMsg) const override;
   void invalidate_trigger_references() const override;

   task_ptr add_task(const std::string& task_name);
   family_ptr add_family(const std::string& family_name);
 	void addTask(  const task_ptr& , size_t position = std::numeric_limits<std::size_t>::max());
 	void addFamily(const family_ptr&,size_t position = std::numeric_limits<std::size_t>::max());
 	void add_child(const node_ptr&,  size_t position = std::numeric_limits<std::size_t>::max());

	void immediateChildren(std::vector<node_ptr>&) const override;
 	virtual void allChildren(std::vector<node_ptr>&) const;
 
 	node_ptr findImmediateChild(const std::string& name,size_t& child_pos) const override;
   node_ptr find_node_up_the_tree(const std::string& name) const override;

	node_ptr find_relative_node(const std::vector<std::string>& pathToNode) override;
	void find_closest_matching_node( const std::vector< std::string >& pathToNode, int indexIntoPathNode, node_ptr& closest_matching_node );

   node_ptr find_by_name(const std::string& name) const;
	family_ptr findFamily(const std::string& familyName) const;
	task_ptr findTask(const std::string& taskName) const;
	void getAllFamilies(std::vector<Family*>&) const;
	void getAllNodes(std::vector<Node*>&) const override;
   void getAllTasks(std::vector<Task*>&) const override;
   void getAllSubmittables(std::vector<Submittable*>&) const override;
   void get_all_active_submittables(std::vector<Submittable*>&) const override;
   void get_all_tasks(std::vector<task_ptr>&) const override;
   void get_all_nodes(std::vector<node_ptr>&) const override;
   void get_all_aliases(std::vector<alias_ptr>&) const override;
	void getAllAstNodes(std::set<Node*>&) const override;
 	const std::vector<node_ptr>& nodeVec() const { return nodes_;}
 	std::vector<task_ptr> taskVec() const;
 	std::vector<family_ptr> familyVec() const;

	void print(std::string&) const override;
	bool operator==(const NodeContainer& rhs) const;

	bool hasTimeDependencies() const override;

	void check_job_creation( job_creation_ctrl_ptr jobCtrl) override;
   void generate_scripts( const std::map<std::string,std::string>& override) const override;

	bool checkInvariants(std::string& errorMsg) const override;
 	void verification(std::string& errorMsg) const override;

	NState::State computedState(Node::TraverseType) const override;

 	node_ptr removeChild( Node* child) override;
 	bool addChild( const node_ptr& child,size_t position = std::numeric_limits<std::size_t>::max()) override;
 	bool isAddChildOk( Node* child, std::string& errorMsg) const override;

 	void setRepeatToLastValueHierarchically() override;
	void setStateOnlyHierarchically(NState::State s,bool force = false) override;
	void set_state_hierarchically(NState::State s, bool force) override;
   void update_limits() override;
   void sort_attributes(ecf::Attr::Type attr,bool recursive) override;

   void archive();
   void restore();
   std::string archive_path() const; //can throw if ECF_HOME not defined

   boost::posix_time::time_duration sum_runtime() override;
   
private:
   void restore_on_begin_or_requeue();

   size_t child_position(const Node*) const override;
   void add_task_only(const task_ptr& ,size_t position = std::numeric_limits<std::size_t>::max());
   void add_family_only(const family_ptr& ,size_t position = std::numeric_limits<std::size_t>::max());

	void handle_defstatus_propagation();
	void match_closest_children(const std::vector<std::string>& pathToNode, int indexIntoPathToNode,node_ptr& closest_matching_node);

	void handleStateChange() override; // called when a state change happens

	friend class Defs;
	friend class Family;
  	bool doDeleteChild(Node* child) override;

	/// For use by python interface,
	std::vector<node_ptr>::const_iterator node_begin() const { return nodes_.begin();}
	std::vector<node_ptr>::const_iterator node_end() const { return nodes_.end();}
	friend void export_SuiteAndFamily();

protected:
   unsigned int order_state_change_no_{0};     // no need to persist
   unsigned int add_remove_state_change_no_{0};// no need to persist

   void force_sync() override;

   void incremental_changes( DefsDelta& changes, compound_memento_ptr& comp) const;

private:
   void copy(const NodeContainer& rhs);
   void swap(NodeContainer& rhs);

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );

private:
  	std::vector<node_ptr> nodes_;
};

#endif
