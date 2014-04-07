/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #50 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include "Node.hpp"
#include "Ecf.hpp"

using namespace ecf;
using namespace std;

void Node::addVariable(const Variable& v )
{
   state_change_no_ = Ecf::incr_state_change_no();

   const std::string& variable_name = v.name();
   size_t theSize = varVec_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (varVec_[i].name() == variable_name) {
         // Variable already exist, *UPDATE* its value
         varVec_[i].set_value(v.theValue());
         if (0 == Ecf::debug_level())
            std::cout << "Node::addVariable: Variable of name '" << v.name() << "' already exist for node " << debugNodePath() << " updating with value '" << v.theValue() << "'\n";
         return;
      }
   }

   if (varVec_.capacity() == 0) varVec_.reserve(5);
	varVec_.push_back( v );
}

void Node::add_variable(const std::string& name, const std::string& value )
{
	addVariable( Variable(name, value) );
}

void Node::add_variable_int(const std::string& name, int some_int )
{
	std::string value =  boost::lexical_cast<std::string>(some_int);
	addVariable( Variable(name, value) );
}

void Node::add_trigger(const std::string& string_expression)
{
	add_trigger_expression(Expression(string_expression));
}
void Node::add_complete(const std::string& string_expression)
{
	add_complete_expression(Expression(string_expression));
}
void Node::add_trigger_expr(const Expression& expr)
{
	add_trigger_expression(expr);
}
void Node::add_complete_expr(const Expression& expr)
{
	add_complete_expression(expr);
}
void Node::add_trigger_expression(const Expression& t)
{
	if ( triggerExpr_ ) {
		std::stringstream ss;
		ss << "Node::add_trigger_expression. A Node(" << absNodePath() << " can only have one trigger ";
		ss << "to add large triggers use multiple calls to Node::add_part_trigger( PartExpression('t1 == complete') )";
 		throw std::runtime_error( ss.str() );
	}
  	triggerExpr_ = new Expression(t);
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::add_complete_expression(const Expression& t)
{
	if ( completeExpr_ ) {
		std::stringstream ss;
		ss << "Node::add_complete_expression. A Node(" << absNodePath() << " can only have one complete expression ";
		ss << "to add large complete expressions use multiple calls to Node::add_part_complete( PartExpression('t1 == complete') )";
 		throw std::runtime_error( ss.str() );
	}
  	completeExpr_ = new Expression(t);
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::add_part_trigger(const PartExpression& part)
{
	if (!triggerExpr_) triggerExpr_ = new Expression();
	triggerExpr_->add( part );
   state_change_no_ = Ecf::incr_state_change_no();
}
void Node::add_part_complete(const PartExpression& part)
{
	if (!completeExpr_) completeExpr_ = new Expression();
	completeExpr_->add( part );
   state_change_no_ = Ecf::incr_state_change_no();
}


void Node::addTime(const ecf::TimeAttr& t)
{
#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::addTime\n";
#endif

   if (isSuite()) {
      throw std::runtime_error("Can not add time based dependency on a suite");
   }

   if (!time_dep_attrs_) time_dep_attrs_ = new TimeDepAttrs(this);
   time_dep_attrs_->addTime(t);  // will call  Ecf::incr_state_change_no();
}

void Node::addToday(const ecf::TodayAttr& t)
{
#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::addToday()\n";
#endif

   if (isSuite()) {
      throw std::runtime_error("Can not add time based dependency on a suite");
   }

   if (!time_dep_attrs_) time_dep_attrs_ = new TimeDepAttrs(this);
   time_dep_attrs_->addToday(t); // will call  Ecf::incr_state_change_no();
}

void Node::addDate( const DateAttr& d)
{
#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::addDate()\n";
#endif

	// By disallowing what effect would if have on existing suites ?
   if (isSuite()) {
      throw std::runtime_error("Can not add time based dependency on a suite"); // Added at 4.0.2
   }

   if (!time_dep_attrs_) time_dep_attrs_ = new TimeDepAttrs(this);
   time_dep_attrs_->addDate(d); // will call  Ecf::incr_state_change_no();
}

void Node::addDay( const DayAttr& d)
{
#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::addDay\n";
#endif

   // By disallowing what effect would if have on existing suites ?
   if (isSuite()) {
      throw std::runtime_error("Can not add time based dependency on a suite"); // Added at 4.0.2
   }

   if (!time_dep_attrs_) time_dep_attrs_ = new TimeDepAttrs(this);
   time_dep_attrs_->addDay(d); // will call  Ecf::incr_state_change_no();
}

void Node::addCron( const CronAttr& d)
{
#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::addCron()\n";
#endif

	if (d.time().isNULL()) {
		throw std::runtime_error("Node::addCron: The cron is in-complete, no time specified");
	}
	if (d.time().hasIncrement() && !repeat_.empty()) {
		std::stringstream ss;
		ss << "Node::addCron: Node " << absNodePath() << " already has a repeat. Inappropriate to add two looping structures at the same level\n";
		throw std::runtime_error(ss.str());
	}

   if (!time_dep_attrs_) time_dep_attrs_ = new TimeDepAttrs(this);
   time_dep_attrs_->addCron(d); // will call  Ecf::incr_state_change_no();
}


void Node::addLabel( const Label& l)
{
   if (child_attrs_) {
      child_attrs_->addLabel(l); // can throw
      return;
   }
   child_attrs_ = new ChildAttrs(this);
   child_attrs_->addLabel(l);
}

void Node::addMeter( const Meter& m)
{
//   if ( isSuite() ) {
//      throw std::runtime_error("Node::addMeter: Can not add meter to a Suite");
//   }

   if (child_attrs_) {
      child_attrs_->addMeter(m); // can throw
      return;
   }
   child_attrs_ = new ChildAttrs(this);
   child_attrs_->addMeter(m);
}

void Node::addEvent( const Event& e)
{
   if (child_attrs_) {
      child_attrs_->addEvent(e); // can throw
      return;
   }
   child_attrs_ = new ChildAttrs(this);
   child_attrs_->addEvent(e);
}

void Node::addLimit(const Limit& l )
{
	if (findLimit(l)) {
		std::stringstream ss;
		ss << "Add Limit failed: Duplicate Limit of name '" << l.name() << "' already exist for node " << debugNodePath();
		throw std::runtime_error( ss.str() );
	}
	limit_ptr the_limit = boost::make_shared<Limit>(l);
	the_limit->set_node(this);
	limitVec_.push_back( the_limit );
   state_change_no_ = Ecf::incr_state_change_no();
}

static void throwIfRepeatAllreadyExists(Node* node)
{
	if (!node->repeat().empty()) {
		std::stringstream ss;
		ss << "Add Repeat failed: Repeat of name '" << node->repeat().name() << "' already exist for node " << node->debugNodePath();
		throw std::runtime_error( ss.str() );
 	}

	if (!node->crons().empty()) {
		std::stringstream ss;
		ss << "Node::addRepeat: Node " << node->absNodePath() << " already has a cron. Inappropriate to add two looping structures at the same level\n";
		throw std::runtime_error(ss.str());
	}
}
void Node::addRepeat( const Repeat& r ){
	throwIfRepeatAllreadyExists(this);
 	repeat_ = Repeat(r);
 	repeat_.update_repeat_genvar();
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::addAutoCancel( const AutoCancelAttr& ac)
{
	if (autoCancel_) {
		std::stringstream ss;
		ss << "Node::addAutoCancel: A node can only have one Autocancel, see node " << debugNodePath();
		throw std::runtime_error( ss.str() );
	}
	autoCancel_ = new ecf::AutoCancelAttr(ac);
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::addLate( const ecf::LateAttr& l )
{
	if (! lateAttr_) {
		lateAttr_ = new ecf::LateAttr(l);
	   state_change_no_ = Ecf::incr_state_change_no();
		return;
	}
 	throw std::runtime_error("Add Late failed: A node can only have one Late attribute, see node " + debugNodePath() );
}


void Node::addVerify( const VerifyAttr& v )
{
   if (misc_attrs_) {
      misc_attrs_->addVerify(v); // can throw
      return;
   }
   misc_attrs_ =  new MiscAttrs(this);
   misc_attrs_->addVerify(v);
}

void Node::addZombie( const ZombieAttr& z)
{
#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Node::addZombie()\n";
#endif

   if (misc_attrs_) {
      misc_attrs_->addZombie(z); // can throw
      return;
   }
   misc_attrs_ = new MiscAttrs(this);
   misc_attrs_->addZombie(z);

}
