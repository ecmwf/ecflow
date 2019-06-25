//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #135 $ 
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

#include <limits>
#include <cassert>
#include <sstream>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "NodeContainer.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "Host.hpp"
#include "JobsParam.hpp"
#include "NodeTreeVisitor.hpp"

#include "Stl.hpp"
#include "Indentor.hpp"
#include "ExprAst.hpp"
#include "NodeState.hpp"
#include "Ecf.hpp"
#include "NodeState.hpp"
#include "SuiteChanged.hpp"
#include "DefsDelta.hpp"
#include "Str.hpp"
#include "Memento.hpp"
#include "Serialization.hpp"

namespace fs = boost::filesystem;
using namespace ecf;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////
//#define DEBUG_FIND_NODE 1
//#define DEBUG_JOB_SUBMISSION 1

/////////////////////////////////////////////////////////////////////////////////////////
NodeContainer::NodeContainer( const std::string& name, bool check)
: Node(name,check),order_state_change_no_(0), add_remove_state_change_no_(0) {}

NodeContainer::NodeContainer() = default;

void NodeContainer::copy(const NodeContainer& rhs)
{
   size_t theSize = rhs.nodes_.size();
   for(size_t s = 0; s < theSize; s++) {
      Task* task = rhs.nodes_[s]->isTask();
      if ( task ) {
         task_ptr task_copy = std::make_shared<Task>( *task );
         task_copy->set_parent(this);
         nodes_.push_back(task_copy);
      }
      else {
         Family* family = rhs.nodes_[s]->isFamily();
         assert(family);
         family_ptr family_copy = std::make_shared<Family>( *family );
         family_copy->set_parent(this);
         nodes_.push_back(family_copy);
      }
   }
}

NodeContainer::NodeContainer(const NodeContainer& rhs )
  : Node(rhs),
    order_state_change_no_(0),
    add_remove_state_change_no_(0)
{
   copy(rhs);
}

NodeContainer& NodeContainer::operator=(const NodeContainer& rhs)
{
   if (this != &rhs) {
      Node::operator=(rhs);
      nodes_.clear();
      copy(rhs);
      order_state_change_no_ = 0;
      add_remove_state_change_no_ = Ecf::incr_state_change_no();
   }
   return *this;
}

NodeContainer::~NodeContainer() = default;

bool NodeContainer::check_defaults() const
{
   if (order_state_change_no_ != 0) throw std::runtime_error("NodeContainer::check_defaults(): order_state_change_no_ != 0");
   if (add_remove_state_change_no_ != 0) throw std::runtime_error("NodeContainer::check_defaults(): add_remove_state_change_no_ != 0");
   return Node::check_defaults();
}

void NodeContainer::accept(ecf::NodeTreeVisitor& v)
{
	v.visitNodeContainer(this);
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++)   { nodes_[t]->accept(v); }
}

void NodeContainer::acceptVisitTraversor(ecf::NodeTreeVisitor& v)
{
	v.visitNodeContainer(this);
}

void NodeContainer::begin()
{
   restore_on_begin_or_requeue();
	Node::begin();
 	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++)   { nodes_[t]->begin(); }
 	handle_defstatus_propagation();
}

void NodeContainer::reset()
{
   Node::reset();
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)   { nodes_[t]->reset(); }
}

void NodeContainer::requeue(Requeue_args& args)
{
//	LOG(Log::DBG,"   " << debugType() << "::requeue() " << absNodePath() << " resetRepeats = " << resetRepeats);

   restore_on_begin_or_requeue();
   Node::requeue(args);

	// For negative numbers, do nothing, i.e do not clear
	if (args.clear_suspended_in_child_nodes_ >= 0) args.clear_suspended_in_child_nodes_++;

	// If the defstatus is complete, don't bother logging the state change in the re-queue
	// When we have several thousands children (as in operations), we will end up
	// changing state to queue, then again changing it back to complete.
	// To avoid this problem we don't bother logging state change for re-queue
	// See: ECFLOW-1239
	if (d_st_ == DState::COMPLETE )
	   args.log_state_changes_ = false;

   Node::Requeue_args largs(true /* reset repeats, Moot for tasks */,
                           args.clear_suspended_in_child_nodes_,
                           args.reset_next_time_slot_,
                           true /* reset relative duration */,
                           args.log_state_changes_);

 	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++) {
 	   nodes_[t]->requeue(largs);
 	}

   handle_defstatus_propagation();
}

void NodeContainer::requeue_time_attrs()
{
   Node::requeue_time_attrs();
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++) {
      nodes_[t]->requeue_time_attrs();
   }
}

void NodeContainer::handle_defstatus_propagation()
{
   if ( d_st_ == DState::COMPLETE ) {
       /// A defstatus of complete and *ONLY* complete should always be applied
       /// hierarchically downwards
       setStateOnlyHierarchically(NState::COMPLETE);
    }
    else if ( d_st_ == DState::default_state() ) {
       /// Reflect that the status of the children.
       /// *However* do NOT override the defstatus setting
       NState::State theSignificantStateOfImmediateChildren = computedState( Node::IMMEDIATE_CHILDREN );
       if ( theSignificantStateOfImmediateChildren != state()) {
          setStateOnly( theSignificantStateOfImmediateChildren );
       }
    }
}

bool NodeContainer::run(JobsParam& jobsParam, bool force)
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++)     { (void) nodes_[t]->run(jobsParam,force); }
	return jobsParam.getErrorMsg().empty();
}

void NodeContainer::kill(const std::string& /* zombie_pid, only valid for single task */)
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {     nodes_[t]->kill(); }
}

void NodeContainer::status()
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {     nodes_[t]->status(); }
}

bool NodeContainer::top_down_why(std::vector<std::string>& theReasonWhy,bool html_tags) const
{
   bool why_found = Node::why(theReasonWhy,true/*top down*/,html_tags);
   if (!why_found) {
      size_t node_vec_size = nodes_.size();
      for(size_t t = 0; t < node_vec_size; t++)   {
         if (nodes_[t]->top_down_why(theReasonWhy,html_tags)) {
            why_found = true;
         }
      }
   }
   return why_found;
}

void NodeContainer::incremental_changes( DefsDelta& changes, compound_memento_ptr& comp) const
{
   /// There no point doing a OrderMemento if children have been added/delete
   if (add_remove_state_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
      comp->add( std::make_shared<ChildrenMemento>( nodes_ ) );
   }
   else if (order_state_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
      std::vector<std::string> order_vec; order_vec.reserve(nodes_.size());
      size_t node_vec_size = nodes_.size();
      for(size_t i =0; i < node_vec_size; i++)  order_vec.push_back( nodes_[i]->name());
      comp->add( std::make_shared<OrderMemento>( order_vec ) );
   }

   Node::incremental_changes(changes, comp);
}

void NodeContainer::set_memento( const OrderMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
   std::cout << "NodeContainer::set_memento( const OrderMemento* ) " << debugNodePath() << "\n";
#endif
   if (aspect_only) {
      aspects.push_back(ecf::Aspect::ORDER);
      return;
   }
   
   // Order nodes_ according to memento ordering
   const std::vector<std::string>& order = memento->order_;
   if (order.size() != nodes_.size()) {
      // something gone wrong.
      std::cout << "NodeContainer::set_memento OrderMemento, memento.size() " << order.size() << " Not the same as nodes_size() " << nodes_.size() << "\n";
      return;
   }

   std::vector<node_ptr> vec; vec.reserve(nodes_.size());
   size_t node_vec_size = nodes_.size();
   for(const auto & i : order) {
      for(size_t t = 0; t < node_vec_size; t++) {
          if (i == nodes_[t]->name()) {
             vec.push_back(nodes_[t]);
             break;
          }
       }
   }
   if (vec.size() !=  nodes_.size()) {
       std::cout << "NodeContainer::set_memento could not find all the names\n";
       return;
   }

   nodes_ = vec;
}

void NodeContainer::set_memento( const ChildrenMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
   std::cout << "NodeContainer::set_memento( const ChildrenMemento * ) " << debugNodePath() << "\n";
#endif
   if (aspect_only) {
      aspects.push_back(ecf::Aspect::ADD_REMOVE_NODE);
      return;
   }

   // setup child parent pointers
   nodes_ = memento->children_;
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodes_[t]->set_parent(this);
   }
}


void NodeContainer::collateChanges(DefsDelta& changes) const
{
   /// Theres no point in traversing children if we have added/removed children
   /// since ChildrenMemento will copy all children.
   if (add_remove_state_change_no_ > changes.client_state_change_no()) {
      return;
   }

	// Traversal to children
 	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++)   { nodes_[t]->collateChanges(changes); }
}

void NodeContainer::order(Node* immediateChild, NOrder::Order ord)
{
	SuiteChanged1 changed(suite());
	switch (ord) {
		case NOrder::TOP:  {
			for(auto i = nodes_.begin(); i != nodes_.end(); ++i) {
 				if ((*i).get() == immediateChild) {
 					node_ptr node = (*i);
					nodes_.erase(i);
					nodes_.insert(nodes_.begin(),node);
               order_state_change_no_ = Ecf::incr_state_change_no();
					return;
 				}
			}
			throw std::runtime_error("NodeContainer::order TOP, immediate child not found");
		}
		case NOrder::BOTTOM:  {
			for(auto i = nodes_.begin(); i != nodes_.end(); ++i) {
 				if ((*i).get() == immediateChild) {
 					node_ptr node = (*i);
					nodes_.erase(i);
					nodes_.push_back(node);
               order_state_change_no_ = Ecf::incr_state_change_no();
					return;
 				}
			}
         throw std::runtime_error("NodeContainer::order BOTTOM, immediate child not found");
		}
		case NOrder::ALPHA:  {
			std::sort(nodes_.begin(),nodes_.end(),
			          [](const node_ptr& a,const node_ptr& b){ return Str::caseInsLess(a->name(),b->name());});
         order_state_change_no_ = Ecf::incr_state_change_no();
			break;
		}
		case NOrder::ORDER:  {
			std::sort(nodes_.begin(),nodes_.end(),
                   [](const node_ptr& a,const node_ptr& b){ return Str::caseInsGreater(a->name(),b->name());});
         order_state_change_no_ = Ecf::incr_state_change_no();
			break;
		}
		case NOrder::UP:  {
			for(size_t t = 0; t  < nodes_.size();t++) {
				if ( nodes_[t].get() == immediateChild) {
					if (t != 0) {
						node_ptr node =  nodes_[t];
						nodes_.erase(nodes_.begin()+t);
						t--;
						nodes_.insert(nodes_.begin()+t,node);
                  order_state_change_no_ = Ecf::incr_state_change_no();
				    }
					return;
 				}
			}
         throw std::runtime_error("NodeContainer::order UP, immediate child not found");
		}
		case NOrder::DOWN: {
		   for(size_t t = 0; t  < nodes_.size();t++) {
		      if ( nodes_[t].get() == immediateChild) {
		         if (t != nodes_.size()-1) {
		            node_ptr node =  nodes_[t];
		            nodes_.erase(nodes_.begin()+t);
		            t++;
		            nodes_.insert(nodes_.begin()+t,node);
		            order_state_change_no_ = Ecf::incr_state_change_no();
		         }
		         return;
		      }
		   }
         throw std::runtime_error("NodeContainer::order DOWN, immediate child not found");
		}
	}
}

void NodeContainer::calendarChanged(
         const ecf::Calendar& c,
         std::vector<node_ptr>& auto_cancelled_nodes,
         std::vector<node_ptr>& auto_archive_nodes,
         const ecf::LateAttr* inherited_late)
{
   // A node that is archived should not allow any change of state.
   if (get_flag().is_set(ecf::Flag::ARCHIVED)) {
      return;
   }

   // The late attribute is inherited, we only set late on the task/alias
	Node::calendarChanged(c,auto_cancelled_nodes,auto_archive_nodes,nullptr);

	LateAttr overridden_late;
   if (inherited_late && !inherited_late->isNull()) {
      overridden_late = *inherited_late;
   }
	if (late_.get() != inherited_late) {
	   overridden_late.override_with(late_.get());
	}

 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
	   nodes_[t]->calendarChanged(c,auto_cancelled_nodes,auto_archive_nodes,&overridden_late);
	}
}

bool NodeContainer::hasAutoCancel() const
{
	if (Node::hasAutoCancel()) return true;
 	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++)     { if (nodes_[t]->hasAutoCancel()) return true; }
	return false;
}

void NodeContainer::invalidate_trigger_references() const
{
   Node::invalidate_trigger_references();
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++) {  nodes_[t]->invalidate_trigger_references(); }
}

bool NodeContainer::resolveDependencies(JobsParam& jobsParam)
{
	// Don't evaluate children unless parent is free. BOMB out early for this case.
	// Note:: Task::resolveDependencies() will check inLimit up front. *** THIS CHECKS UP THE HIERARCHY ***
	// Note:: Node::resolveDependencies() may have forced family node to complete, should have have
	//        returned false in this case, to stop any job submission
	if ( ! Node::resolveDependencies(jobsParam) ) {

#ifdef DEBUG_JOB_SUBMISSION
		LOG(Log::DBG, "   NodeContainer::resolveDependencies " << absNodePath() << " could not resolve dependencies, may have completed");
		cout << "NodeContainer::resolveDependencies " << absNodePath() << " could not resolve dependencies may have completed" << endl;
#endif
		return false;
	}

   /// During *top down* traversal we check in limits at this level. Done here rather than
	/// in Node::resolveDependencies(). Otherwise this particular check will get duplicated
	/// since the task, will do *bottom up* traversal.
   if (!check_in_limit()) {
#ifdef DEBUG_DEPENDENCIES
      LOG(Log::DBG,"   NodeContainer::resolveDependencies() " << absNodePath() << " HOLDING due to inLIMIT");
#endif
      return false;
   }

	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++) {
 		// Note: we don't bomb out early here. Since a later child could be free. i.e f1/ty or t4
 		// child t1 holding
 		// child t2 holding
 		// child f1 free
 		//   child tx holding
 		//   child ty free
 		// child t3 holding
 		// child t4 free
  		(void) nodes_[t]->resolveDependencies(jobsParam) ;
 	}
 	return true;
}

bool NodeContainer::has_time_based_attributes() const
{
   if (Node::has_time_based_attributes()) return true;
   for(const auto& node: nodes_) {
      if (node->has_time_based_attributes()) return true;
   }
   return false;
}

NState::State NodeContainer::computedState(Node::TraverseType traverseType) const
{
	if (nodes_.empty()) {
		/// Note: theComputedNodeState will return unknown if no children, in this
		/// case just return the current state.
		return state();
	}

	// returns the computed state depending on traverseType
	// If not IMMEDIATE_CHILDREN, will recurse down calling each child's computedState() function
  	return ecf::theComputedNodeState(nodes_, (traverseType == Node::IMMEDIATE_CHILDREN) );
}

void NodeContainer::force_sync()
{
   add_remove_state_change_no_ = Ecf::incr_state_change_no();
}

node_ptr NodeContainer::removeChild(Node* child)
{
 	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++)     {
 		if (nodes_[t].get() == child) {
 			node_ptr node = std::dynamic_pointer_cast<Node>(nodes_[t]);
 			child->set_parent(nullptr); // must set to NULL, allows it to be re-added to different parent
 			nodes_.erase( nodes_.begin() + t);
 			add_remove_state_change_no_ = Ecf::incr_state_change_no();
 			return node ;
 		}
  	}
	// Should never happen
 	LOG_ASSERT(false,"NodeContainer::removeChild: Could not remove child");
	return node_ptr();
}

bool NodeContainer::addChild( const node_ptr& child, size_t position)
{
	// *** CANT construct shared_ptr from a raw pointer, must use dynamic_pointer_cast,
	// *** otherwise the reference counts will get messed up.
	try {
		if ( child->isTask() ) {
			// can throw if duplicate names
			addTask( std::dynamic_pointer_cast<Task>(child), position );
			return true;
		}

		if ( child->isFamily() ) {
			// can throw if duplicate names
			addFamily( std::dynamic_pointer_cast<Family>(child), position );
			return true;
		}
	}
	catch  ( std::runtime_error &e) {}

	// Duplicate names, or trying to add a Suite?
	return false;
}

bool NodeContainer::isAddChildOk( Node* theChild, std::string& errorMsg) const
{
	Task* theTaskChild = theChild->isTask();
	if ( theTaskChild ) {

		node_ptr theTask = find_by_name(theChild->name());
		if (!theTask.get())  return true;

		std::stringstream ss;
		ss << "Task/Family of name " << theChild->name() << " already exist in container node " << name() ;
		errorMsg += ss.str();
		return false;
 	}

	Family* theFamilyChild = theChild->isFamily();
	if ( theFamilyChild ) {

	   node_ptr theFamily = find_by_name(theChild->name());
		if (!theFamily.get())  return true;

		std::stringstream ss;
		ss << "Family/Task of name " << theChild->name() << " already exist in container node " << name() ;
		errorMsg += ss.str();
		return false;
 	}

	Suite* theSuite = theChild->isSuite();
	if ( theSuite ) {
		errorMsg += "Can not add a suite as child.";
		return false;
	}

	errorMsg += "Unknown node type";
	return false;
}

void NodeContainer::handleStateChange()
{
	// Increment any repeats & requeue
	requeueOrSetMostSignificantStateUpNodeTree();

   Node::handleStateChange(); // may do a autorestore, if state is COMPLETE
}

size_t NodeContainer::child_position(const Node* child) const
{
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)     {
      if (nodes_[t].get() == child) {
         return t;
      }
   }
   return std::numeric_limits<std::size_t>::max();
}

task_ptr NodeContainer::add_task(const std::string& task_name)
{
   if (find_by_name(task_name).get()) {
      std::stringstream ss;
      ss << "Add Task failed: A task/family of name '" << task_name << "' already exist on node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   task_ptr the_task = Task::create(task_name);
   add_task_only(the_task);
   return the_task;
}

family_ptr NodeContainer::add_family(const std::string& family_name)
{
   if (find_by_name(family_name).get()) {
      std::stringstream ss;
      ss << "Add Family failed: A Family/Task of name '" << family_name << "' already exist on node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   family_ptr the_family = Family::create(family_name);
   add_family_only( the_family );
   return the_family;
}

void NodeContainer::addTask(const task_ptr& t,size_t position)
{
	if (find_by_name(t->name()).get()) {
		std::stringstream ss;
		ss << "Add Task failed: A Task/Family of name '" << t->name() << "' already exist on node " << debugNodePath();
		throw std::runtime_error( ss.str() );
	}
	add_task_only( t, position);
}

void NodeContainer::add_task_only( const task_ptr& t, size_t position)
{
   if (t->parent()) {
      std::stringstream ss;
      ss << debugNodePath() << ": Add Task failed: A task of name '" << t->name() << "' is already owned by another node" ;
      throw std::runtime_error( ss.str() );
   }

   t->set_parent(this);
   if (position >= nodes_.size()) {
      nodes_.push_back( t );
   }
   else {
      nodes_.insert( nodes_.begin() + position, t);
   }
   add_remove_state_change_no_ = Ecf::incr_state_change_no();
}

void NodeContainer::add_family_only( const family_ptr& f, size_t position)
{
   if (f->parent()) {
      std::stringstream ss;
      ss << debugNodePath() << ": Add Family failed: A family of name '" << f->name() << "' is already owned by another node";
      throw std::runtime_error( ss.str() );
   }

   f->set_parent(this);
   if (position >= nodes_.size()) {
      nodes_.push_back( f );
   }
   else {
      nodes_.insert( nodes_.begin() + position, f);
   }
   add_remove_state_change_no_ = Ecf::incr_state_change_no();
}


void NodeContainer::addFamily(const family_ptr& f,size_t position)
{
	if (find_by_name(f->name()).get()) {
		std::stringstream ss;
		ss << "Add Family failed: A Family/Task of name '" << f->name() << "' already exist on node " << debugNodePath();
		throw std::runtime_error( ss.str() );
	}
	add_family_only( f, position );
}

void NodeContainer::add_child(const node_ptr& child,size_t position)
{
   if (child->isTask()) {
      task_ptr task_child = std::dynamic_pointer_cast<Task>( child );
      addTask(task_child,position);
   }
   else if (child->isFamily()) {
       family_ptr family_child = std::dynamic_pointer_cast<Family>( child );
       addFamily(family_child,position);
   }
}

node_ptr NodeContainer::findImmediateChild(const std::string& theName, size_t& child_pos) const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
		if (nodes_[t]->name() == theName) {
		   child_pos = t;
	 		return nodes_[t];
 		}
 	}
   child_pos = std::numeric_limits<std::size_t>::max();
	return node_ptr();
}

node_ptr NodeContainer::find_node_up_the_tree(const std::string& the_name) const
{
   if (name() == the_name) {
      return non_const_this();
   }

   size_t not_used;
   node_ptr fnd_node = findImmediateChild(the_name,not_used);
   if (fnd_node) return fnd_node;

   Node* the_parent = parent();
   if (the_parent) return the_parent->find_node_up_the_tree(the_name);
   return node_ptr();
}


node_ptr NodeContainer::find_relative_node( const std::vector< std::string >& pathToNode)
{
#ifdef DEBUG_FIND_NODE
	 cout << "NodeContainer::find_relative_node for '" << name() << "\n";
	 cout << " path :";
	 BOOST_FOREACH(const std::string& s,pathToNode ) { cout << " " << s;} cout << "\n";
    BOOST_FOREACH(node_ptr t, nodes_ ) { cout << " " << t->name();} cout << "\n";
#endif
 	if (pathToNode.empty())  return node_ptr();
	auto pathSize = static_cast<int>(pathToNode.size());

#ifdef DEBUG_FIND_NODE
	cout << "NodeContainer::find_relative_node name = '" << name() << "' pathToNode[0] = '" << pathToNode[0] << "'\n";
#endif

 	// Must match all children
	int index = 0;
 	size_t child_pos = 0 ; // unused
 	node_ptr the_node = shared_from_this();
 	while (index < pathSize) {
      the_node = the_node->findImmediateChild(pathToNode[index],child_pos);
      if (the_node) {
         if (index == pathSize - 1) return the_node;
         index++;
      }
      else {
         return node_ptr();
      }
 	}
 	return node_ptr();
}

void NodeContainer::find_closest_matching_node( const std::vector< std::string >& pathToNode, int indexIntoPathNode, node_ptr& closest_matching_node )
{
	auto pathSize = static_cast<int>(pathToNode.size());
	if (indexIntoPathNode >= pathSize)  return;

	int index = indexIntoPathNode;
 	if (name() == pathToNode[indexIntoPathNode]) {

 		closest_matching_node = shared_from_this();

 		// Match the Container i.e family or suite
 		bool lastIndex = ( indexIntoPathNode == pathSize - 1);
 		if ( lastIndex ) {
 			return;
		}

		// Match the Children, i.e. go down the hierarchy
		index++;
		match_closest_children(pathToNode,index,closest_matching_node);
 	}
}

void NodeContainer::match_closest_children(const std::vector<std::string>& pathToNode, int indexIntoPathNode, node_ptr& closest_matching_node)
{
	auto pathSize = static_cast<int>(pathToNode.size());
	if (indexIntoPathNode >= pathSize)  return;

	bool lastIndex = ( indexIntoPathNode == pathSize - 1);
	if ( lastIndex ) {
		// even if the name matches, its only valid if the index is the last index
		// i.e if we have a suite like /a/b/c/d/e
		//     and a path like         /a/b/c/d/e/f/g
		// In this we will match with e but it not valid since its not the last index
		size_t task_vec_size = nodes_.size();
		for(size_t t = 0; t < task_vec_size; t++) {
			if (nodes_[t]->name() == pathToNode[indexIntoPathNode]) {
				closest_matching_node = nodes_[t];
				return;
			}
		}
	}
	else {
		// Path to node is of the form "/family/task" or "/family/family/task"
		// Path to node is of the form "/suite/task" or "/suite/family/task"
		size_t family_vec_size = nodes_.size();
		for(size_t f = 0; f < family_vec_size; f++) {
			Family* family = nodes_[f]->isFamily();
			if (family) {
				node_ptr matching_node;
				family->find_closest_matching_node(pathToNode, indexIntoPathNode,matching_node);
				if (matching_node.get()) {
					closest_matching_node = matching_node;
					return;
				}
			}
		}
	}
}

node_ptr NodeContainer::find_by_name(const std::string& name) const
{
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++) {
      if (nodes_[t]->name() == name) {
         return nodes_[t];
      }
   }
   return node_ptr();
}

family_ptr NodeContainer::findFamily(const std::string& familyName) const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t f = 0; f < node_vec_size; f++) {
 		if (nodes_[f]->name() == familyName && nodes_[f]->isFamily()) {
	 		return std::dynamic_pointer_cast<Family>(nodes_[f]);
 		}
	}
	return family_ptr();
}

task_ptr NodeContainer::findTask(const std::string& taskName) const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
 		if (nodes_[t]->name() == taskName && nodes_[t]->isTask()) {
	 		return std::dynamic_pointer_cast<Task>(nodes_[t]);
 		}
	}
	return task_ptr();
}

bool NodeContainer::hasTimeDependencies() const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++)  { if (nodes_[t]->hasTimeDependencies()) return true;}
 	return false;
}

void NodeContainer::immediateChildren(std::vector<node_ptr>& theChildren) const
{
	size_t task_vec_size = nodes_.size();
	theChildren.reserve( theChildren.size() + task_vec_size);
	for(size_t t = 0; t < task_vec_size; t++) {
 		theChildren.push_back( nodes_[t] );
	}
}

void NodeContainer::allChildren(std::vector<node_ptr>& vec) const
{
 	size_t node_vec_size = nodes_.size();
 	for(size_t f = 0; f < node_vec_size; f++) {
		vec.push_back(nodeVec_[f]);
		nodeVec_[f]->allChildren(vec); // for task does nothing
	}
}

void NodeContainer::getAllFamilies(std::vector<Family*>& vec) const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t f = 0; f < node_vec_size; f++) {
		Family* family =  nodes_[f]->isFamily();
		if ( family ) {
			vec.push_back(family);
			family->getAllFamilies(vec);
		}
	}
}

void NodeContainer::getAllNodes(std::vector<Node*>& vec) const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
	   vec.push_back(nodes_[t].get());
	   nodes_[t]->getAllNodes(vec);
	}
}

void NodeContainer::getAllTasks(std::vector<Task*>& tasks) const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {
	   nodes_[t]->getAllTasks(tasks);
	}
}

void NodeContainer::getAllSubmittables(std::vector<Submittable*>& tasks) const
{
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodes_[t]->getAllSubmittables(tasks);
   }
}

void NodeContainer::get_all_active_submittables(std::vector<Submittable*>& tasks) const
{
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodes_[t]->get_all_active_submittables(tasks);
   }
}

void NodeContainer::get_all_tasks(std::vector<task_ptr>& tasks) const
{
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodes_[t]->get_all_tasks(tasks);
   }
}

void NodeContainer::get_all_nodes(std::vector<node_ptr>& nodes) const
{
   nodes.push_back(non_const_this());
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodes_[t]->get_all_nodes(nodes);
   }
}

void NodeContainer::get_all_aliases(std::vector<alias_ptr>& aliases) const
{
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodes_[t]->get_all_aliases(aliases);
   }
}

void NodeContainer::getAllAstNodes(std::set<Node*>& vec) const
{
	Node::getAllAstNodes(vec);
 	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++)     { nodes_[t]->getAllAstNodes(vec); }
}

bool NodeContainer::check(std::string& errorMsg, std::string& warningMsg) const
{
	Node::check(errorMsg, warningMsg);

	// recursive to handle hierarchical families
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
 		nodes_[t]->check(errorMsg,warningMsg);
      // if (!errorMsg.empty()) break;
  	}

	return errorMsg.empty();
}

std::vector<task_ptr> NodeContainer::taskVec() const
{
 	size_t node_vec_size = nodes_.size();
	std::vector<task_ptr> vec; vec.reserve(node_vec_size);
	for(size_t t = 0; t < node_vec_size; t++)   {
 		if (nodes_[t]->isTask()) {
 			vec.push_back( std::dynamic_pointer_cast<Task>(nodes_[t]) );
		}
	}
	return vec;
}

std::vector<family_ptr> NodeContainer::familyVec() const
{
	std::vector<family_ptr> vec;
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {
 		if (nodes_[t]->isFamily()) {
 			vec.push_back( std::dynamic_pointer_cast<Family>(nodes_[t]) );
		}
	}
	return vec;
}

bool NodeContainer::operator==(const NodeContainer& rhs) const
{
   size_t node_vec_size = nodes_.size();
   if ( node_vec_size != rhs.nodes_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "NodeContainer::operator==  node_vec_size != rhs.nodes_.size() " << absNodePath() << "\n";
         std::cout << "   nodes_.size() = " << node_vec_size << "  rhs.nodes_.size() = " << rhs.nodes_.size() << "\n";
      }
#endif
      return false;
   }

   for(size_t i =0; i < node_vec_size; ++i) {

      Task* task = nodes_[i]->isTask();
      if (task) {
         Task* rhs_task = rhs.nodes_[i]->isTask();
         if ( !rhs_task ) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
               std::cout << "NodeContainer::operator==  if ( !rhs_task ) " << absNodePath() << "\n";
            }
#endif
            return false;
         }

         if ( !( *task == *rhs_task )) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
               std::cout << "NodeContainer::operator==  if ( !( *task == *rhs_task )) " << absNodePath() << "\n";
            }
#endif
            return false;
         }
      }
      else {
         Family* rhs_family = rhs.nodes_[i]->isFamily();
         if ( !rhs_family ) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
               std::cout << "NodeContainer::operator==  if ( !rhs_family ) " << absNodePath() << "\n";
            }
#endif
            return false;
         }

         Family* family = nodes_[i]->isFamily(); LOG_ASSERT( family, "" );
         if ( family/*keep clang happy*/ && !( *family == *rhs_family )) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
               std::cout << "NodeContainer::operator==  if ( !( *family == *rhs_family )) " << absNodePath() << "\n";
            }
#endif
            return false;
         }
      }
   }

   return Node::operator==(rhs);
}

void NodeContainer::print(std::string& os) const
{
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++) { nodes_[t]->print( os ); }
}

bool NodeContainer::checkInvariants(std::string& errorMsg) const
{
   if (!Node::checkInvariants(errorMsg)) return false;

   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++) {
      if (nodes_[t]->parent() != this) {
         errorMsg += "NodeContainer::checkInvariants family/task parent() not correct";
         return false;
      }
      if (!nodes_[t]->checkInvariants(errorMsg)) {
         return false;
      }
   }
   return true;
}

void NodeContainer::verification(std::string& errorMsg) const
{
 	Node::verification(errorMsg);
 	size_t node_vec_size = nodes_.size();
 	for(size_t t = 0; t < node_vec_size; t++)  { nodes_[t]->verification(errorMsg); }
}

void NodeContainer::setRepeatToLastValueHierarchically()
{
	setRepeatToLastValue();
 	size_t node_vec_size = nodes_.size();
  	for(size_t t = 0; t < node_vec_size; t++) { nodes_[t]->setRepeatToLastValueHierarchically(); }
}

void NodeContainer::setStateOnlyHierarchically(NState::State s,bool force)
{
	setStateOnly(s,force);
 	size_t node_vec_size = nodes_.size();
	for(size_t t = 0; t < node_vec_size; t++) { nodes_[t]->setStateOnlyHierarchically(s,force); }
}

void NodeContainer::set_state_hierarchically(NState::State s, bool force)
{
	setStateOnlyHierarchically(s,force);
	if (force) {
	   // *force* is only set via ForceCmd.
	   update_limits();  // hierarchical
	}
	handleStateChange(); // non-hierarchical
}

void NodeContainer::update_limits()
{
   /// Only tasks can affect the limits, hence no point calling locally
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++) { nodes_[t]->update_limits(); }
}


std::string NodeContainer::archive_path() const
{
   std::string the_archive_path;
   if (!findParentUserVariableValue( Str::ECF_HOME() , the_archive_path )) {
      std::stringstream ss; ss << "NodeContainer::archive_path: can not find ECF_HOME from " << debugNodePath();
      throw std::runtime_error(ss.str());
   }

   std::string the_archive_file_name = absNodePath();
   Str::replaceall(the_archive_file_name,"/",":"); // we use ':' since its not allowed in the node names
   the_archive_file_name += ".check";

   std::string port = Str::DEFAULT_PORT_NUMBER();
   Defs* the_defs = defs();
   if ( the_defs ) {
      port = the_defs->server().find_variable(Str::ECF_PORT());
      if ( port.empty() ) port = Str::DEFAULT_PORT_NUMBER();
   }
   Host host;
   the_archive_file_name = host.prefix_host_and_port(port,the_archive_file_name);

   the_archive_path += "/";
   the_archive_path += the_archive_file_name;
   return the_archive_path;
}

void NodeContainer::archive()
{
   if (nodes_.empty()) return; // nothing to archive

   // make a clone of this node DEEP COPY
   node_ptr this_clone = clone();

   // re-create node tree up to the def. Do *NOT* clone we just need a SHALLOW hierarchy
   defs_ptr archive_defs = Defs::create();
   if (isSuite()) {
      suite_ptr suite_clone = std::dynamic_pointer_cast<Suite>(this_clone);
      archive_defs->addSuite(suite_clone);
   }
   else {
      Node* parent_ptr = parent();
      while(parent_ptr) {
         if (parent_ptr->isSuite()) {
            suite_ptr parent_suite = Suite::create(parent_ptr->name());
            parent_suite->addChild(this_clone);
            archive_defs->addSuite(parent_suite);
            break;
         }
         else {
            family_ptr parent_family = Family::create(parent_ptr->name());
            parent_family->addChild(this_clone);
            this_clone = parent_family;
         }
         parent_ptr = parent_ptr->parent();
      }
   }

   // save the created defs, to disk
   archive_defs->save_as_checkpt(archive_path());

   flag().set(ecf::Flag::ARCHIVED);    // flag as archived
   flag().clear(ecf::Flag::RESTORED);

   // delete the child nodes, set parent to null first.
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++) { nodes_[t]->set_parent(nullptr); }
   nodes_.clear();

   std::vector<node_ptr>().swap(nodes_);                    // reclaim vector memory
   add_remove_state_change_no_ = Ecf::incr_state_change_no(); // For sync
   string msg = "autoarchive "; msg += debugNodePath();       // inform user via log
   ecf::log(Log::MSG,msg);
}

void NodeContainer::swap(NodeContainer& rhs)
{
   std::swap(nodes_,rhs.nodes_);
   size_t theSize = nodes_.size();
   for(size_t s = 0; s < theSize; s++) {
      nodes_[s]->set_parent(this);
   }
}

void NodeContainer::restore_on_begin_or_requeue()
{
   if (!flag().is_set(ecf::Flag::ARCHIVED)) return;
   if (!nodes_.empty()) return;
   if (!fs::exists(archive_path())) return;

   // Node::requeue(...) will clear ecf::Flag::RESTORED, set in restore()
   try { restore(); }
   catch(std::exception&  e) {
      std::stringstream ss; ss << "NodeContainer::restore_on_begin_or_requeue(): failed : " << e.what();
      log(Log::ERR,ss.str());
   }
}

void NodeContainer::restore()
{
   if (!flag().is_set(ecf::Flag::ARCHIVED)) {
      std::stringstream ss; ss << "NodeContainer::restore() Node " << absNodePath() << " can't restore, ecf::Flag::ARCHIVED not set";
      throw std::runtime_error(ss.str());
   }

   if (!nodes_.empty()) {
      std::stringstream ss; ss << "NodeContainer::restore() Node " << absNodePath() << " can't restore, Container already has children ?";
      throw std::runtime_error(ss.str());
   }

   defs_ptr archive_defs = Defs::create();
   std::string the_archive_path = archive_path();
   try { archive_defs->restore(the_archive_path);}
   catch(std::exception& e) {
       std::stringstream ss; ss << "NodeContainer::restore() Node " << absNodePath() << " could not restore file at  " << the_archive_path << "  : " << e.what();
       throw std::runtime_error(ss.str());
   }

   // find the same node in the defs.
   node_ptr archived_node = archive_defs->findAbsNode(absNodePath());
   if (!archived_node) {
      std::stringstream ss; ss << "NodeContainer::restore() could not find " << absNodePath() << " in the archived file " << the_archive_path;
      throw std::runtime_error(ss.str());
   }
   NodeContainer* archived_node_container = archived_node->isNodeContainer();
   if (!archived_node_container) {
       std::stringstream ss; ss << "NodeContainer::restore() The node at " << absNodePath() << " recovered from " << the_archive_path << " is not a container(suite/family)";
       throw std::runtime_error(ss.str());
   }

   swap(*archived_node_container);                           // swap the children, and set parent pointers
   flag().clear(ecf::Flag::ARCHIVED);                        // clear flag archived
   flag().set(ecf::Flag::RESTORED);                          // set restored flag, to stop automatic autoarchive
   add_remove_state_change_no_ = Ecf::incr_state_change_no();// For sync
   fs::remove(the_archive_path);                             // remove the file
}


void NodeContainer::sort_attributes(ecf::Attr::Type attr,bool recursive)
{
   Node::sort_attributes(attr,recursive);
   if (recursive) {
      size_t node_vec_size = nodes_.size();
      for(size_t t = 0; t < node_vec_size; t++) { nodes_[t]->sort_attributes(attr,recursive); }
   }
}

bool NodeContainer::doDeleteChild(Node* child)
{
	SuiteChanged1 changed(suite());
	auto theTaskEnd = nodes_.end();
 	for(auto t = nodes_.begin(); t!=theTaskEnd; ++t) {
 		if ( (*t).get() == child) {
 		   child->set_parent(nullptr); // must set to NULL, allow it to be re-added to different parent
  			nodes_.erase(t);
         add_remove_state_change_no_ = Ecf::incr_state_change_no();
         set_most_significant_state_up_node_tree();
  			return true;
 		}
		if ((*t)->doDeleteChild(child)) {
 			return true;
 		}
 	}

 	return false;
}

void NodeContainer::check_job_creation( job_creation_ctrl_ptr jobCtrl) {

	if (defStatus() != DState::COMPLETE) {
	 	size_t node_vec_size = nodes_.size();
		for(size_t t = 0; t < node_vec_size; t++) { nodes_[t]->check_job_creation( jobCtrl ); }
 	}
}

void NodeContainer::generate_scripts( const std::map<std::string,std::string>& override) const
{
   size_t node_vec_size = nodes_.size();
   for(size_t t = 0; t < node_vec_size; t++) {
      nodes_[t]->generate_scripts( override );
   }
}



template<class Archive>
void NodeContainer::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Node>(this),
      CEREAL_NVP(nodes_));

   // Setup the parent pointers. Since they are not serialised
   if (Archive::is_loading::value) {
      size_t vec_size = nodes_.size();
      for(size_t i = 0; i < vec_size; i++) {
         nodes_[i]->set_parent(this);
      }
   }
}
CEREAL_TEMPLATE_SPECIALIZE_V(NodeContainer);
