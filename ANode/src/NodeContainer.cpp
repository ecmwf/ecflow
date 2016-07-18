//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #135 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <limits>
#include <assert.h>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include "NodeContainer.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Defs.hpp"
#include "Log.hpp"
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

using namespace ecf;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////
//#define DEBUG_FIND_NODE 1
//#define DEBUG_JOB_SUBMISSION 1

/////////////////////////////////////////////////////////////////////////////////////////
NodeContainer::NodeContainer( const std::string& name )
: Node(name),order_state_change_no_(0), add_remove_state_change_no_(0) {}

NodeContainer::NodeContainer()
: order_state_change_no_(0),add_remove_state_change_no_(0) {}

NodeContainer::NodeContainer(const NodeContainer& rhs )
  : Node(rhs),
    order_state_change_no_(0),
    add_remove_state_change_no_(0)
{
   size_t theSize = rhs.nodeVec_.size();
   for(size_t s = 0; s < theSize; s++) {
      Task* task = rhs.nodeVec_[s]->isTask();
      if ( task ) {
         task_ptr task_copy = boost::make_shared<Task>( *task );
         task_copy->set_parent(this);
         nodeVec_.push_back(task_copy);
      }
      else {
         Family* family = rhs.nodeVec_[s]->isFamily();
         assert(family);
         family_ptr family_copy = boost::make_shared<Family>( *family );
         family_copy->set_parent(this);
         nodeVec_.push_back(family_copy);
      }
   }
}

NodeContainer::~NodeContainer() {}

void NodeContainer::accept(ecf::NodeTreeVisitor& v)
{
	v.visitNodeContainer(this);
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)   { nodeVec_[t]->accept(v); }
}

void NodeContainer::acceptVisitTraversor(ecf::NodeTreeVisitor& v)
{
	v.visitNodeContainer(this);
}

void NodeContainer::begin()
{
	Node::begin();
 	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++)   { nodeVec_[t]->begin(); }
 	handle_defstatus_propagation();
}

void NodeContainer::requeue(
         bool resetRepeats,
         int clear_suspended_in_child_nodes,
         bool reset_next_time_slot
         )
{
//	LOG(Log::DBG,"   " << debugType() << "::requeue() " << absNodePath() << " resetRepeats = " << resetRepeats);
	Node::requeue(resetRepeats,clear_suspended_in_child_nodes,reset_next_time_slot);

	// For negative numbers, do nothing, i.e do not clear
	if (clear_suspended_in_child_nodes >=0) clear_suspended_in_child_nodes++;

 	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++) {
 	   nodeVec_[t]->requeue(true /*reset child repeats. Moot for tasks*/,
 	                        clear_suspended_in_child_nodes,
 	                        reset_next_time_slot);
 	}
   handle_defstatus_propagation();
}

void NodeContainer::requeue_time_attrs()
{
   Node::requeue_time_attrs();
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++) {
      nodeVec_[t]->requeue_time_attrs();
   }
}

void NodeContainer::handle_defstatus_propagation()
{
   if ( defStatus_ == DState::COMPLETE ) {
       /// A defstatus of complete and *ONLY* complete should always be applied
       /// hierarchically downwards
       setStateOnlyHierarchically(NState::COMPLETE);
    }
    else if ( defStatus_ == DState::default_state() ) {
       /// Reflect that the status of the children.
       /// *However* do NOT override the defstatus setting
       NState::State theSignificantStateOfImmediateChildren = computedState( Node::IMMEDIATE_CHILDREN );
       if ( theSignificantStateOfImmediateChildren != state()) {
          setStateOnly( theSignificantStateOfImmediateChildren );
       }
    }
}

void NodeContainer::resetRelativeDuration()
{
	Node::resetRelativeDuration();
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)     { nodeVec_[t]->resetRelativeDuration(); }
}

bool NodeContainer::run(JobsParam& jobsParam, bool force)
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)     { (void) nodeVec_[t]->run(jobsParam,force); }
	return jobsParam.getErrorMsg().empty();
}

void NodeContainer::kill(const std::string& /* zombie_pid, only valid for single task */)
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {     nodeVec_[t]->kill(); }
}

void NodeContainer::status()
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {     nodeVec_[t]->status(); }
}

void NodeContainer::top_down_why(std::vector<std::string>& theReasonWhy) const
{
	Node::why(theReasonWhy);
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {     nodeVec_[t]->top_down_why(theReasonWhy); }
}

void NodeContainer::incremental_changes( DefsDelta& changes, compound_memento_ptr& comp) const
{
   /// There no point doing a OrderMemento if children have been added/delete
   if (add_remove_state_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = boost::make_shared<CompoundMemento>(absNodePath());
      comp->add( boost::make_shared<ChildrenMemento>( nodeVec_ ) );
   }
   else if (order_state_change_no_ > changes.client_state_change_no()) {
      if (!comp.get()) comp = boost::make_shared<CompoundMemento>(absNodePath());
      std::vector<std::string> order_vec; order_vec.reserve(nodeVec_.size());
      size_t node_vec_size = nodeVec_.size();
      for(size_t i =0; i < node_vec_size; i++)  order_vec.push_back( nodeVec_[i]->name());
      comp->add( boost::make_shared<OrderMemento>( order_vec ) );
   }

   Node::incremental_changes(changes, comp);
}

void NodeContainer::set_memento( const OrderMemento* memento,std::vector<ecf::Aspect::Type>& aspects ) {
#ifdef DEBUG_MEMENTO
   std::cout << "NodeContainer::set_memento( const OrderMemento* ) " << debugNodePath() << "\n";
#endif

   // Order nodeVec_ according to memento ordering
   const std::vector<std::string>& order = memento->order_;
   if (order.size() != nodeVec_.size()) {
      // something gone wrong.
      std::cout << "NodeContainer::set_memento OrderMemento, memento.size() " << order.size() << " Not the same as nodeVec_size() " << nodeVec_.size() << "\n";
      return;
   }

   std::vector<node_ptr> vec; vec.reserve(nodeVec_.size());
   size_t node_vec_size = nodeVec_.size();
   for(size_t i = 0; i < order.size(); i++) {
      for(size_t t = 0; t < node_vec_size; t++) {
          if (order[i] == nodeVec_[t]->name()) {
             vec.push_back(nodeVec_[t]);
             break;
          }
       }
   }
   if (vec.size() !=  nodeVec_.size()) {
       std::cout << "NodeContainer::set_memento could not find all the names\n";
       return;
   }

   aspects.push_back(ecf::Aspect::ORDER);
   nodeVec_ = vec;
}

void NodeContainer::set_memento( const ChildrenMemento* memento,std::vector<ecf::Aspect::Type>& aspects ) {
#ifdef DEBUG_MEMENTO
   std::cout << "NodeContainer::set_memento( const OrderMemento* ) " << debugNodePath() << "\n";
#endif
   aspects.push_back(ecf::Aspect::ADD_REMOVE_NODE);
   nodeVec_ = memento->children_;

   // setup child parent pointers
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodeVec_[t]->set_parent(this);
   }
}


void NodeContainer::collateChanges(DefsDelta& changes) const
{
   /// There no point in traversing children of we have added/removed children
   /// since ChildrenMemento will copy all children.
   if (add_remove_state_change_no_ > changes.client_state_change_no()) {
      return;
   }

	// Traversal to children
 	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++)   { nodeVec_[t]->collateChanges(changes); }
}

void NodeContainer::order(Node* immediateChild, NOrder::Order ord)
{
	SuiteChanged1 changed(suite());
	switch (ord) {
		case NOrder::TOP:  {
			for(std::vector<node_ptr>::iterator i = nodeVec_.begin(); i != nodeVec_.end(); ++i) {
 				if ((*i).get() == immediateChild) {
 					node_ptr node = (*i);
					nodeVec_.erase(i);
					nodeVec_.insert(nodeVec_.begin(),node);
               order_state_change_no_ = Ecf::incr_state_change_no();
					return;
 				}
			}
			throw std::runtime_error("NodeContainer::order TOP, immediate child not found");
		}
		case NOrder::BOTTOM:  {
			for(std::vector<node_ptr>::iterator i = nodeVec_.begin(); i != nodeVec_.end(); ++i) {
 				if ((*i).get() == immediateChild) {
 					node_ptr node = (*i);
					nodeVec_.erase(i);
					nodeVec_.push_back(node);
               order_state_change_no_ = Ecf::incr_state_change_no();
					return;
 				}
			}
         throw std::runtime_error("NodeContainer::order BOTTOM, immediate child not found");
		}
		case NOrder::ALPHA:  {
			std::sort(nodeVec_.begin(),nodeVec_.end(),
			            boost::bind(Str::caseInsLess,
			                          boost::bind(&Node::name,_1),
			                          boost::bind(&Node::name,_2)));
         order_state_change_no_ = Ecf::incr_state_change_no();
			break;
		}
		case NOrder::ORDER:  {
			std::sort(nodeVec_.begin(),nodeVec_.end(),
			            boost::bind(Str::caseInsGreater,
			                          boost::bind(&Node::name,_1),
			                          boost::bind(&Node::name,_2)));
         order_state_change_no_ = Ecf::incr_state_change_no();
			break;
		}
		case NOrder::UP:  {
			for(size_t t = 0; t  < nodeVec_.size();t++) {
				if ( nodeVec_[t].get() == immediateChild) {
					if (t != 0) {
						node_ptr node =  nodeVec_[t];
						nodeVec_.erase(nodeVec_.begin()+t);
						t--;
						nodeVec_.insert(nodeVec_.begin()+t,node);
                  order_state_change_no_ = Ecf::incr_state_change_no();
				    }
					return;
 				}
			}
         throw std::runtime_error("NodeContainer::order UP, immediate child not found");
		}
		case NOrder::DOWN: {
		   for(size_t t = 0; t  < nodeVec_.size();t++) {
		      if ( nodeVec_[t].get() == immediateChild) {
		         if (t != nodeVec_.size()-1) {
		            node_ptr node =  nodeVec_[t];
		            nodeVec_.erase(nodeVec_.begin()+t);
		            t++;
		            nodeVec_.insert(nodeVec_.begin()+t,node);
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
         const ecf::LateAttr* inherited_late)
{
   // The late attribute is inherited, we only set late on the task/alias
	Node::calendarChanged(c,auto_cancelled_nodes,NULL);


	LateAttr overridden_late;
   if (inherited_late && !inherited_late->isNull()) {
      overridden_late = *inherited_late;
   }
	if (lateAttr_ != inherited_late) {
	   overridden_late.override_with(lateAttr_);
	}


 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
	   nodeVec_[t]->calendarChanged(c,auto_cancelled_nodes,&overridden_late);
	}
}

bool NodeContainer::hasAutoCancel() const
{
	if (Node::hasAutoCancel()) return true;
 	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++)     { if (nodeVec_[t]->hasAutoCancel()) return true; }
	return false;
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

	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++) {
 		// Note: we don't bomb out early here. Since a later child could be free. i.e f1/ty or t4
 		// child t1 holding
 		// child t2 holding
 		// child f1 free
 		//   child tx holding
 		//   child ty free
 		// child t3 holding
 		// child t4 free
  		(void) nodeVec_[t]->resolveDependencies(jobsParam) ;
 	}
 	return true;
}

NState::State NodeContainer::computedState(Node::TraverseType traverseType) const
{
	if (nodeVec_.empty()) {
		/// Note: theComputedNodeState will return unknown if no children, in this
		/// case just return the current state.
		return state();
	}

	// returns the computed state depending on traverseType
	// If not IMMEDIATE_CHILDREN, will recurse down calling each child's computedState() function
  	return ecf::theComputedNodeState(nodeVec_, (traverseType == Node::IMMEDIATE_CHILDREN) );
}

node_ptr NodeContainer::removeChild(Node* child)
{
 	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++)     {
 		if (nodeVec_[t].get() == child) {
 			node_ptr node = boost::dynamic_pointer_cast<Node>(nodeVec_[t]);
 			child->set_parent(NULL); // must set to NULL, allows it to be re-added to different parent
 			nodeVec_.erase( nodeVec_.begin() + t);
 			add_remove_state_change_no_ = Ecf::incr_state_change_no();
 			return node ;
 		}
  	}
	// Should never happen
 	LOG_ASSERT(false,"NodeContainer::removeChild: Could not remove child");
	return node_ptr();
}

bool NodeContainer::addChild( node_ptr child, size_t position)
{
	// *** CANT construct shared_ptr from a raw pointer, must use dynamic_pointer_cast,
	// *** otherwise the reference counts will get messed up.
	try {
		if ( child->isTask() ) {
			// can throw if duplicate names
			addTask( boost::dynamic_pointer_cast<Task>(child), position );
			return true;
		}

		if ( child->isFamily() ) {
			// can throw if duplicate names
			addFamily( boost::dynamic_pointer_cast<Family>(child), position );
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

		task_ptr theTask = findTask(theChild->name());
		if (!theTask.get())  return true;

		std::stringstream ss;
		ss << "Task of name " << theChild->name() << " already exist in container node " << name() ;
		errorMsg += ss.str();
		return false;
 	}

	Family* theFamilyChild = theChild->isFamily();
	if ( theFamilyChild ) {

		family_ptr theFamily = findFamily(theChild->name());
		if (!theFamily.get())  return true;

		std::stringstream ss;
		ss << "Family of name " << theChild->name() << " already exist in container node " << name() ;
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
}

size_t NodeContainer::child_position(const Node* child) const
{
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++)     {
      if (nodeVec_[t].get() == child) {
         return t;
      }
   }
   return std::numeric_limits<std::size_t>::max();
}

task_ptr NodeContainer::add_task(const std::string& task_name)
{
   if (findTask(task_name).get()) {
      std::stringstream ss;
      ss << "Add Task failed: A task of name '" << task_name << "' already exist on node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   task_ptr the_task = Task::create(task_name);
   add_task_only(the_task);
   return the_task;
}

family_ptr NodeContainer::add_family(const std::string& family_name)
{
   if (findFamily(family_name).get()) {
      std::stringstream ss;
      ss << "Add Family failed: A Family of name '" << family_name << "' already exist on node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   family_ptr the_family = Family::create(family_name);
   add_family_only( the_family );
   return the_family;
}

void NodeContainer::addTask(task_ptr t,size_t position)
{
	if (findTask(t->name()).get()) {
		std::stringstream ss;
		ss << "Add Task failed: A task of name '" << t->name() << "' already exist on node " << debugNodePath();
		throw std::runtime_error( ss.str() );
	}
	add_task_only( t, position);
}

void NodeContainer::add_task_only( task_ptr t, size_t position)
{
   if (t->parent()) {
      std::stringstream ss;
      ss << debugNodePath() << ": Add Task failed: A task of name '" << t->name() << "' is already owned by another node" ;
      throw std::runtime_error( ss.str() );
   }

   t->set_parent(this);
   if (position >= nodeVec_.size()) {
      nodeVec_.push_back( t );
   }
   else {
      nodeVec_.insert( nodeVec_.begin() + position, t);
   }
   add_remove_state_change_no_ = Ecf::incr_state_change_no();
}

void NodeContainer::add_family_only( family_ptr f, size_t position)
{
   if (f->parent()) {
      std::stringstream ss;
      ss << debugNodePath() << ": Add Family failed: A family of name '" << f->name() << "' is already owned by another node";
      throw std::runtime_error( ss.str() );
   }

   f->set_parent(this);
   if (position >= nodeVec_.size()) {
      nodeVec_.push_back( f );
   }
   else {
      nodeVec_.insert( nodeVec_.begin() + position, f);
   }
   add_remove_state_change_no_ = Ecf::incr_state_change_no();
}


void NodeContainer::addFamily(family_ptr f,size_t position)
{
	if (findFamily(f->name()).get()) {
		std::stringstream ss;
		ss << "Add Family failed: A Family of name '" << f->name() << "' already exist on node " << debugNodePath();
		throw std::runtime_error( ss.str() );
	}
	add_family_only( f, position );
}

node_ptr NodeContainer::findImmediateChild(const std::string& theName, size_t& child_pos) const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
		if (nodeVec_[t]->name() == theName) {
		   child_pos = t;
	 		return nodeVec_[t];
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
//#ifdef DEBUG_FIND_NODE
//	cout << "NodeContainer::find_relative_node for '" << name()
//		 << "' path = '" << pathToNode << "\n";
//    cout << " tasks = ";  BOOST_FOREACH(task_ptr t, nodeVec_ ) { cout << " " << t->name(); } cout << "\n";
//    cout << " family = "; BOOST_FOREACH(family_ptr f, familyVec_ ) { cout << " " << f->name();} cout << "\n";
//#endif
 	if (pathToNode.empty())  return node_ptr();
	int pathSize = static_cast<int>(pathToNode.size());

#ifdef DEBUG_FIND_NODE
	cout << "NodeContainer::find_relative_node name = '" << name() << "' pathToNode[0] = '" << pathToNode[0] << "'\n";
#endif

 	if (pathSize == 1 && name() == pathToNode[0]) {
 		// Match the Container i.e family or suite
   	return shared_from_this();
 	}

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
	int pathSize = static_cast<int>(pathToNode.size());
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
	int pathSize = static_cast<int>(pathToNode.size());
	if (indexIntoPathNode >= pathSize)  return;

	bool lastIndex = ( indexIntoPathNode == pathSize - 1);
	if ( lastIndex ) {
		// even if the name matches, its only valid if the index is the last index
		// i.e if we have a suite like /a/b/c/d/e
		//     and a path like         /a/b/c/d/e/f/g
		// In this we will match with e but it not valid since its not the last index
		size_t task_vec_size = nodeVec_.size();
		for(size_t t = 0; t < task_vec_size; t++) {
			if (nodeVec_[t]->name() == pathToNode[indexIntoPathNode]) {
				closest_matching_node = nodeVec_[t];
				return;
			}
		}
	}
	else {
		// Path to node is of the form "/family/task" or "/family/family/task"
		// Path to node is of the form "/suite/task" or "/suite/family/task"
		size_t family_vec_size = nodeVec_.size();
		for(size_t f = 0; f < family_vec_size; f++) {
			Family* family = nodeVec_[f]->isFamily();
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

family_ptr NodeContainer::findFamily(const std::string& familyName) const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t f = 0; f < node_vec_size; f++) {
 		if (nodeVec_[f]->name() == familyName && nodeVec_[f]->isFamily()) {
	 		return boost::dynamic_pointer_cast<Family>(nodeVec_[f]);
 		}
	}
	return family_ptr();
}

task_ptr NodeContainer::findTask(const std::string& taskName) const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
 		if (nodeVec_[t]->name() == taskName && nodeVec_[t]->isTask()) {
	 		return boost::dynamic_pointer_cast<Task>(nodeVec_[t]);
 		}
	}
	return task_ptr();
}

bool NodeContainer::hasTimeDependencies() const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)  { if (nodeVec_[t]->hasTimeDependencies()) return true;}
 	return false;
}

void NodeContainer::immediateChildren(std::vector<node_ptr>& theChildren) const
{
	size_t task_vec_size = nodeVec_.size();
	theChildren.reserve( theChildren.size() + task_vec_size);
	for(size_t t = 0; t < task_vec_size; t++) {
 		theChildren.push_back( nodeVec_[t] );
	}
}

void NodeContainer::allChildren(std::set<Node*>& theSet) const
{
 	size_t node_vec_size = nodeVec_.size();
 	for(size_t f = 0; f < node_vec_size; f++) {
		theSet.insert(nodeVec_[f].get());
		nodeVec_[f]->allChildren(theSet);
	}
}

void NodeContainer::getAllFamilies(std::vector<Family*>& vec) const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t f = 0; f < node_vec_size; f++) {
		Family* family =  nodeVec_[f]->isFamily();
		if ( family ) {
			vec.push_back(family);
			family->getAllFamilies(vec);
		}
	}
}

void NodeContainer::getAllNodes(std::vector<Node*>& vec) const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
	   vec.push_back(nodeVec_[t].get());
	   nodeVec_[t]->getAllNodes(vec);
	}
}

void NodeContainer::getAllTasks(std::vector<Task*>& tasks) const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {
	   nodeVec_[t]->getAllTasks(tasks);
	}
}

void NodeContainer::getAllSubmittables(std::vector<Submittable*>& tasks) const
{
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodeVec_[t]->getAllSubmittables(tasks);
   }
}

void NodeContainer::get_all_active_submittables(std::vector<Submittable*>& tasks) const
{
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodeVec_[t]->get_all_active_submittables(tasks);
   }
}

void NodeContainer::get_all_tasks(std::vector<task_ptr>& tasks) const
{
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodeVec_[t]->get_all_tasks(tasks);
   }
}

void NodeContainer::get_all_nodes(std::vector<node_ptr>& nodes) const
{
   nodes.push_back(non_const_this());
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodeVec_[t]->get_all_nodes(nodes);
   }
}

void NodeContainer::get_all_aliases(std::vector<alias_ptr>& aliases) const
{
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++)   {
      nodeVec_[t]->get_all_aliases(aliases);
   }
}

void NodeContainer::getAllAstNodes(std::set<Node*>& vec) const
{
	Node::getAllAstNodes(vec);
 	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++)     { nodeVec_[t]->getAllAstNodes(vec); }
}

bool NodeContainer::check(std::string& errorMsg, std::string& warningMsg) const
{
	Node::check(errorMsg, warningMsg);

	// recursive to handle hierarchical families
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++) {
 		nodeVec_[t]->check(errorMsg,warningMsg);
      // if (!errorMsg.empty()) break;
  	}

	return errorMsg.empty();
}

std::vector<task_ptr> NodeContainer::taskVec() const
{
 	size_t node_vec_size = nodeVec_.size();
	std::vector<task_ptr> vec; vec.reserve(node_vec_size);
	for(size_t t = 0; t < node_vec_size; t++)   {
 		if (nodeVec_[t]->isTask()) {
 			vec.push_back( boost::dynamic_pointer_cast<Task>(nodeVec_[t]) );
		}
	}
	return vec;
}

std::vector<family_ptr> NodeContainer::familyVec() const
{
	std::vector<family_ptr> vec;
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++)   {
 		if (nodeVec_[t]->isFamily()) {
 			vec.push_back( boost::dynamic_pointer_cast<Family>(nodeVec_[t]) );
		}
	}
	return vec;
}

bool NodeContainer::operator==(const NodeContainer& rhs) const
{
	size_t node_vec_size = nodeVec_.size();
	if ( node_vec_size != rhs.nodeVec_.size()) {
#ifdef DEBUG
		if (Ecf::debug_equality()) {
			std::cout << "NodeContainer::operator==  node_vec_size != rhs.nodeVec_.size() " << absNodePath() << "\n";
			std::cout << "   nodeVec_.size() = " << node_vec_size << "  rhs.nodeVec_.size() = " << rhs.nodeVec_.size() << "\n";
		}
#endif
 		return false;
	}

	for(size_t i =0; i < node_vec_size; ++i) {

		Task* task = nodeVec_[i]->isTask();
		if (task) {
			Task* rhs_task = rhs.nodeVec_[i]->isTask();
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
			Family* rhs_family = rhs.nodeVec_[i]->isFamily();
			if ( !rhs_family ) {
#ifdef DEBUG
				if (Ecf::debug_equality()) {
					std::cout << "NodeContainer::operator==  if ( !rhs_family ) " << absNodePath() << "\n";
				}
#endif
				return false;
			}

			Family* family = nodeVec_[i]->isFamily(); LOG_ASSERT( family, "" );
			if ( !( *family == *rhs_family )) {
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

std::ostream& NodeContainer::print(std::ostream& os) const
{
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++) { nodeVec_[t]->print( os ); }
 	return os;
}

bool NodeContainer::checkInvariants(std::string& errorMsg) const
{
   if (!Node::checkInvariants(errorMsg)) return false;

   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++) {
      if (nodeVec_[t]->parent() != this) {
         errorMsg += "NodeContainer::checkInvariants family/task parent() not correct";
         return false;
      }
      if (!nodeVec_[t]->checkInvariants(errorMsg)) {
         return false;
      }
   }
   return true;
}

void NodeContainer::verification(std::string& errorMsg) const
{
 	Node::verification(errorMsg);
 	size_t node_vec_size = nodeVec_.size();
 	for(size_t t = 0; t < node_vec_size; t++)  { nodeVec_[t]->verification(errorMsg); }
}

void NodeContainer::setRepeatToLastValueHierarchically()
{
	setRepeatToLastValue();
 	size_t node_vec_size = nodeVec_.size();
  	for(size_t t = 0; t < node_vec_size; t++) { nodeVec_[t]->setRepeatToLastValueHierarchically(); }
}

void NodeContainer::setStateOnlyHierarchically(NState::State s,bool force)
{
	setStateOnly(s,force);
 	size_t node_vec_size = nodeVec_.size();
	for(size_t t = 0; t < node_vec_size; t++) { nodeVec_[t]->setStateOnlyHierarchically(s,force); }
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
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++) { nodeVec_[t]->update_limits(); }
}

bool NodeContainer::doDeleteChild(Node* child)
{
	SuiteChanged1 changed(suite());
	std::vector<node_ptr>::iterator theTaskEnd = nodeVec_.end();
 	for(std::vector<node_ptr>::iterator t = nodeVec_.begin(); t!=theTaskEnd; ++t) {
 		if ( (*t).get() == child) {
 		   child->set_parent(NULL); // must set to NULL, allow it to be re-added to different parent
  			nodeVec_.erase(t);
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
	 	size_t node_vec_size = nodeVec_.size();
		for(size_t t = 0; t < node_vec_size; t++) { nodeVec_[t]->check_job_creation( jobCtrl ); }
 	}
}

void NodeContainer::generate_scripts( const std::map<std::string,std::string>& override) const
{
   size_t node_vec_size = nodeVec_.size();
   for(size_t t = 0; t < node_vec_size; t++) {
      nodeVec_[t]->generate_scripts( override );
   }
}
