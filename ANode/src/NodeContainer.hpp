#ifndef NODE_CONTAINER_HPP_
#define NODE_CONTAINER_HPP_

//============================================================================
// Name        : NodeContainer.hpp
// Author      : Avi
// Revision    : $Revision: #88 $ 
//
// Copyright 2009-2017 ECMWF.
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
#include "CheckPtContext.hpp"
#include "MigrateContext.hpp"

class NodeContainer : public Node {
protected:
   NodeContainer& operator=(const NodeContainer&);
public:
   NodeContainer( const std::string& name );
   NodeContainer(const NodeContainer& );
	NodeContainer();
	virtual ~NodeContainer();

	virtual void accept(ecf::NodeTreeVisitor&);
	virtual void acceptVisitTraversor(ecf::NodeTreeVisitor& v);
    virtual void reset();
	virtual void begin();
	virtual void requeue(const Requeue_args&);
   virtual void requeue_time_attrs();
	virtual bool run(JobsParam& jobsParam, bool force);
	virtual void kill(const std::string& zombie_pid = "");
	virtual void status();
	virtual bool top_down_why(std::vector<std::string>& theReasonWhy,bool html_tags = false) const;
	virtual void collateChanges(DefsDelta&) const;
   void set_memento(const OrderMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   void set_memento(const ChildrenMemento*,std::vector<ecf::Aspect::Type>& aspects,bool f );
   virtual void order(Node* immediateChild, NOrder::Order);

	virtual bool hasAutoCancel() const;
 	virtual void calendarChanged(const ecf::Calendar&,std::vector<node_ptr>& auto_cancelled_nodes,const ecf::LateAttr* inherited_late);
 	virtual bool resolveDependencies(JobsParam& );
 	virtual bool check(std::string& errorMsg, std::string& warningMsg) const;

   task_ptr add_task(const std::string& task_name);
   family_ptr add_family(const std::string& family_name);
 	void addTask( task_ptr , size_t position = std::numeric_limits<std::size_t>::max());
 	void addFamily( family_ptr, size_t position = std::numeric_limits<std::size_t>::max());

	virtual void immediateChildren(std::vector<node_ptr>&) const;
 	virtual void allChildren(std::set<Node*>&) const;

 	virtual node_ptr findImmediateChild(const std::string& name,size_t& child_pos) const;
   virtual node_ptr find_node_up_the_tree(const std::string& name) const;

	virtual node_ptr
 find_relative_node(const std::vector<std::string>& pathToNode);
	void find_closest_matching_node( const std::vector< std::string >& pathToNode, int indexIntoPathNode, node_ptr& closest_matching_node );

	family_ptr findFamily(const std::string& familyName) const;
	task_ptr findTask(const std::string& taskName) const;
	void getAllFamilies(std::vector<Family*>&) const;
	virtual void getAllNodes(std::vector<Node*>&) const;
   virtual void getAllTasks(std::vector<Task*>&) const;
   virtual void getAllSubmittables(std::vector<Submittable*>&) const;
   virtual void get_all_active_submittables(std::vector<Submittable*>&) const;
   virtual void get_all_tasks(std::vector<task_ptr>&) const;
   virtual void get_all_nodes(std::vector<node_ptr>&) const;
   virtual void get_all_aliases(std::vector<alias_ptr>&) const;
	virtual void getAllAstNodes(std::set<Node*>&) const;
 	const std::vector<node_ptr>& nodeVec() const { return nodeVec_;}
 	std::vector<task_ptr> taskVec() const;
 	std::vector<family_ptr> familyVec() const;

	std::ostream& print(std::ostream&) const;
	bool operator==(const NodeContainer& rhs) const;

	virtual bool hasTimeDependencies() const;

	virtual void check_job_creation( job_creation_ctrl_ptr jobCtrl);
   virtual void generate_scripts( const std::map<std::string,std::string>& override) const;

	virtual bool checkInvariants(std::string& errorMsg) const;
 	virtual void verification(std::string& errorMsg) const;

	virtual NState::State computedState(Node::TraverseType) const;

 	virtual node_ptr removeChild( Node* child);
 	virtual bool addChild( node_ptr child,size_t position = std::numeric_limits<std::size_t>::max());
 	virtual bool isAddChildOk( Node* child, std::string& errorMsg) const;

 	virtual void setRepeatToLastValueHierarchically();
	virtual void setStateOnlyHierarchically(NState::State s,bool force = false);
	virtual void set_state_hierarchically(NState::State s, bool force);
   virtual void update_limits();
   virtual void sort_attributes(ecf::Attr::Type attr,bool recursive);

private:
   virtual size_t child_position(const Node*) const;
   void add_task_only( task_ptr ,size_t position = std::numeric_limits<std::size_t>::max());
   void add_family_only( family_ptr ,size_t position = std::numeric_limits<std::size_t>::max());

	void handle_defstatus_propagation();
	void match_closest_children(const std::vector<std::string>& pathToNode, int indexIntoPathToNode,node_ptr& closest_matching_node);

	virtual void handleStateChange(); // called when a state change happens

	friend class Defs;
	friend class Family;
  	virtual bool doDeleteChild(Node* child);

	/// For use by python interface,
	std::vector<node_ptr>::const_iterator node_begin() const { return nodeVec_.begin();}
	std::vector<node_ptr>::const_iterator node_end() const { return nodeVec_.end();}
	friend void export_SuiteAndFamily();

protected:
   unsigned int order_state_change_no_;     // no need to persist
   unsigned int add_remove_state_change_no_;// no need to persist

   virtual void force_sync();

   void incremental_changes( DefsDelta& changes, compound_memento_ptr& comp) const;

private:
   void copy(const NodeContainer& rhs);
	friend class boost::serialization::access;

	// distinguish between check-pointing and server->client comm's
	// *when* handling Flag::MIGRATED.
	// Check-pointing should always persist nodeVec_
	// Flag::MIGRATED should not persist nodeVec_ when in server->client command context
	template<class Archive>
	void serialize(Archive & ar, const unsigned int /*version*/)
	{
	   ar.register_type(static_cast<Task *>(NULL));
	   ar.register_type(static_cast<Family *>(NULL));

	   // serialise base class information
	   ar & boost::serialization::base_object<Node>(*this);

	   // Handle ecf::Flag::MIGRATED, don't save nodeVec_
	   // When check-pointing we always need to save the children
	   if (Archive::is_saving::value &&
	         get_flag().is_set(ecf::Flag::MIGRATED) &&
	         ! ecf::CheckPtContext::in_checkpt() &&
	         ! ecf::MigrateContext::in_migrate()
	       ) {

	      std::vector<node_ptr> nodeVec;
	      ar & nodeVec;
	   }
	   else {
	      ar & nodeVec_;
	   }

      // Setup the parent pointers. Since they are not serialised
      if (Archive::is_loading::value) {
         size_t vec_size = nodeVec_.size();
         for(size_t i = 0; i < vec_size; i++) {
            nodeVec_[i]->set_parent(this);
         }
      }
	}

private:
  	std::vector<node_ptr> nodeVec_;
};

#endif
