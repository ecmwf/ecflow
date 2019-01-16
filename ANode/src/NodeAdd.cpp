/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #50 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/lexical_cast.hpp>
#include "AutoRestoreAttr.hpp"
#include "AutoCancelAttr.hpp"
#include "AutoArchiveAttr.hpp"
#include "Limit.hpp"
#include "Node.hpp"

#include "Ecf.hpp"

using namespace ecf;
using namespace std;

bool Node::update_variable(const std::string& name, const std::string& value)
{
   size_t theSize = vars_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (vars_[i].name() == name) {
         // Variable already exist, *UPDATE* its value
         vars_[i].set_value(value );
         if (0 == Ecf::debug_level())
            std::cout << "Node::addVariable: Variable of name '" << name << "' already exist for node " << debugNodePath() << " updating with value '" << value << "'\n";
         return true;
      }
   }
   return false;
}

void Node::addVariable(const Variable& v )
{
   state_change_no_ = Ecf::incr_state_change_no();
   if (update_variable(v.name(),v.theValue())) return;
   if (vars_.capacity() == 0) vars_.reserve(5);
	vars_.push_back( v );
}

void Node::add_variable(const std::string& name, const std::string& value )
{
   state_change_no_ = Ecf::incr_state_change_no();
   if (update_variable(name,value)) return;
   if (vars_.capacity() == 0) vars_.reserve(5);
   vars_.emplace_back(name,value);
}

void Node::add_variable_bypass_name_check(const std::string& name, const std::string& value )
{
   state_change_no_ = Ecf::incr_state_change_no();
   if (update_variable(name,value)) return;
   if (vars_.capacity() == 0) vars_.reserve(5);
   vars_.emplace_back(name,value,false);
}

void Node::add_variable_int(const std::string& name, int some_int )
{
	std::string value = boost::lexical_cast<std::string>(some_int);
	add_variable(name,value);
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
	if ( t_expr_ ) {
		std::stringstream ss;
		ss << "Node::add_trigger_expression. A Node(" << absNodePath() << " can only have one trigger ";
		ss << "to add large triggers use multiple calls to Node::add_part_trigger( PartExpression('t1 == complete') )";
 		throw std::runtime_error( ss.str() );
	}
	if (isSuite())  throw std::runtime_error( "Can not add trigger on a suite" );

  	t_expr_ = std::make_unique<Expression>(t);
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::add_complete_expression(const Expression& t)
{
	if ( c_expr_ ) {
		std::stringstream ss;
		ss << "Node::add_complete_expression. A Node(" << absNodePath() << " can only have one complete expression ";
		ss << "to add large complete expressions use multiple calls to Node::add_part_complete( PartExpression('t1 == complete') )";
 		throw std::runtime_error( ss.str() );
	}
   if (isSuite())  throw std::runtime_error( "Can not add complete trigger on a suite" );

  	c_expr_ = std::make_unique<Expression>(t);
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::py_add_trigger_expr(const std::vector<PartExpression>& vec)
{
   if (t_expr_) {
      if (isSuite())  throw std::runtime_error( "Can not add trigger on a suite" );
      t_expr_->add_expr(vec);
      state_change_no_ = Ecf::incr_state_change_no();
   }
   else {
      Expression expr;
      for(const auto & i : vec) expr.add(i);
      add_trigger_expression(expr);
   }
}

void Node::py_add_complete_expr( const std::vector<PartExpression>& vec )
{
   if (c_expr_) {
      if (isSuite())  throw std::runtime_error( "Can not add complete on a suite" );
      c_expr_->add_expr(vec);
      state_change_no_ = Ecf::incr_state_change_no();
   }
   else {
      Expression expr;
      for(const auto & i : vec) expr.add(i);
      add_complete_expression(expr);
   }
}

void Node::add_part_trigger(const PartExpression& part)
{
   if (isSuite())  throw std::runtime_error( "Can not add trigger on a suite" );

	if (!t_expr_) t_expr_ = std::make_unique<Expression>();
	t_expr_->add( part );
   state_change_no_ = Ecf::incr_state_change_no();
}
void Node::add_part_complete(const PartExpression& part)
{
   if (isSuite())  throw std::runtime_error( "Can not add complete trigger on a suite" );

	if (!c_expr_) c_expr_ = std::make_unique<Expression>();
	c_expr_->add( part );
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

   times_.push_back(t);
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::addToday(const ecf::TodayAttr& t)
{
#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::addToday()\n";
#endif

   if (isSuite()) {
      throw std::runtime_error("Can not add time based dependency on a suite");
   }

   todays_.push_back(t);
   state_change_no_ = Ecf::incr_state_change_no();
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

   dates_.push_back( d );
   state_change_no_ = Ecf::incr_state_change_no();
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

   days_.push_back( d );
   state_change_no_ = Ecf::incr_state_change_no();
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

	crons_.push_back( d );
	state_change_no_ = Ecf::incr_state_change_no();
}


void Node::addLabel( const Label& l)
{
   if (findLabel(l.name())) {
       std::stringstream ss;
       ss << "Add Label failed: Duplicate label of name '" << l.name() << "' already exist for node " << debugNodePath();
       throw std::runtime_error( ss.str() );
    }
    labels_.push_back( l );
    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::add_label(const std::string& name,const std::string& value, const std::string& new_value)
{
   if (findLabel(name)) {
      std::stringstream ss;
      ss << "Add Label failed: Duplicate label of name '" << name << "' already exist for node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   labels_.emplace_back(name,value,new_value);
   state_change_no_ = Ecf::incr_state_change_no();
}


void Node::addMeter( const Meter& m)
{
//   if ( isSuite() ) {
//      throw std::runtime_error("Node::addMeter: Can not add meter to a Suite");
//   }

   const Meter& meter = findMeter(m.name());
   if (!meter.empty()) {
      std::stringstream ss;
      ss << "Add Meter failed: Duplicate Meter of name '" << m.name() << "' already exist for node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   meters_.push_back( m );
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::addEvent( const Event& e)
{
   const Event& event = findEvent(e);
   if (!event.empty()) {
      std::stringstream ss;
      ss << "Add Event failed: Duplicate Event of name '" << e.name_or_number() << "' already exist for node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   events_.push_back( e );
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::addLimit(const Limit& l )
{
	if (findLimit(l)) {
		std::stringstream ss;
		ss << "Add Limit failed: Duplicate Limit of name '" << l.name() << "' already exist for node " << debugNodePath();
		throw std::runtime_error( ss.str() );
	}
	limit_ptr the_limit = std::make_shared<Limit>(l);
	the_limit->set_node(this);
	limits_.push_back( the_limit );
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::addInLimit(const InLimit& l)
{
   inLimitMgr_.addInLimit(l);
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

void Node::addAutoCancel( const ecf::AutoCancelAttr& ac)
{
   if (auto_archive_) {
       std::stringstream ss;
       ss << "Node::addAutoCancel: Can not add autocancel and autoarchive on the same node " << debugNodePath();
       throw std::runtime_error( ss.str() );
    }
    if (auto_cancel_) {
       std::stringstream ss;
       ss << "Node::addAutoCancel: A node can only have one autocancel, see node " << debugNodePath();
       throw std::runtime_error( ss.str() );
    }
    auto_cancel_ = std::make_unique<ecf::AutoCancelAttr>(ac);
    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::add_autoarchive( const ecf::AutoArchiveAttr& aa)
{
   if (auto_cancel_) {
      std::stringstream ss;
      ss << "Node::add_autoarchive: Can not add autocancel and autoarchive on the same node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   if (auto_archive_) {
      std::stringstream ss;
      ss << "Node::add_autoarchive: A node can only have one autoarchive, see node " << debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   auto_archive_ = std::make_unique<ecf::AutoArchiveAttr>(aa);
   state_change_no_ = Ecf::incr_state_change_no();
}

void Node::add_autorestore( const ecf::AutoRestoreAttr& ar)
{
   if (auto_restore_) {
       std::stringstream ss;
       ss << "Node::add_auto_restore: Can only have one autorestore per node " << debugNodePath();
       throw std::runtime_error( ss.str() );
    }
    auto_restore_ = std::make_unique<ecf::AutoRestoreAttr>(ar);
    auto_restore_->set_node(this);
    state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd
}

void Node::addLate( const ecf::LateAttr& l )
{
	if (! late_) {
		late_ = std::make_unique<ecf::LateAttr>(l);
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
   misc_attrs_ = std::make_unique<MiscAttrs>(this);
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
   misc_attrs_ = std::make_unique<MiscAttrs>(this);
   misc_attrs_->addZombie(z);
}

void Node::add_queue( const QueueAttr& q)
{
   if (!misc_attrs_)  misc_attrs_ = std::make_unique<MiscAttrs>(this);
   misc_attrs_->add_queue(q);
}

void Node::add_generic( const GenericAttr& q)
{
   if (!misc_attrs_)  misc_attrs_ = std::make_unique<MiscAttrs>(this);
   misc_attrs_->add_generic(q);
}
