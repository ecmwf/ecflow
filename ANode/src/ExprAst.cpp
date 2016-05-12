//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #57 $ 
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

#include <vector>
#include <boost/foreach.hpp>

#include "ExprAst.hpp"
#include "Indentor.hpp"
#include "ExprAstVisitor.hpp"
#include "Node.hpp"
#include "Log.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;

////////////////////////////////////////////////////////////////////////////////////

Ast::~Ast() {}

////////////////////////////////////////////////////////////////////////////////////

AstTop::~AstTop() { delete root_;}

void AstTop::accept(ExprAstVisitor& v)
{
	v.visitTop(this);
	root_->accept(v);
}

AstTop* AstTop::clone() const
{
   AstTop* top = new AstTop();
   top->addChild( root_->clone() );
   return top;
}

bool AstTop::evaluate() const
{
	if (root_) {
		return root_->evaluate();
	}

	LOG_ASSERT(false,"AstTop::evaluate(): assert failed, AST top has no root/children");
	return false;
}

bool AstTop::check(std::string& error_msg) const
{
   if (root_) {
      return root_->check(error_msg);
   }
   return true;
}

std::ostream& AstTop::print(std::ostream& os) const
{
	Indentor in;
	Indentor::indent(os) << "# AST\n";
 	if (root_) {
		Indentor in;
		return root_->print(os);
	}
	return os;
}

void AstTop::print_flat(std::ostream& os,bool add_bracket) const
{
   if (root_) {
      root_->print_flat(os,add_bracket);
   }
}

//#define DEBUG_WHY 1
bool AstTop::why(std::string& theReasonWhy) const
{
	if (evaluate()) {
#ifdef DEBUG_WHY
 		std::cout << "   AstTop::why evaluate returning\n";
#endif
		return false;
	}
	return root_->why(theReasonWhy);
}

std::string AstTop::expression(bool why) const
{
	std::string ret =  exprType_;
	if (root_) {
		ret += " ";
		ret += root_->expression(why);
	}
	return ret;
}

void AstTop::setParentNode(Node* p)
{
	if (root_) root_->setParentNode(p);
}

//////////////////////////////////////////////////////////////////////////////////////

AstRoot::~AstRoot() {
	delete left_;
	delete right_;
	left_ = 0;
	right_ = 0;
}

void AstRoot::accept(ExprAstVisitor& v)
{
	v.visitRoot(this);
	left_->accept(v);
	if (right_) right_->accept(v); // right_ is empty for Not
}

bool AstRoot::check(std::string& error_msg) const
{
   if (left_ && !left_->check(error_msg)) return false;
   if (right_ && !right_->check(error_msg)) return false;
   return true;
}

void AstRoot::addChild( Ast* n )
{
	LOG_ASSERT(n,"");

	if ( !left_ ) {
		left_ = n;
		return;
	}
	if ( !right_ ) {
		right_ = n;
		return;
	}

	LOG_ASSERT(false,"AstRoot::addChild: assert failed: root already has left and right children\n");
}

std::ostream& AstRoot::print( std::ostream& os ) const {
	if (left_->isRoot()) {
		Indentor in;
 		left_->print( os );
	}
	else left_->print( os );

	if (right_) { // right_ is empty for Not
		if (right_->isRoot()) {
			Indentor in;
			right_->print( os );
		}
		else right_->print( os ); ;
	}
	return os;
}

bool AstRoot::why(std::string& theReasonWhy) const
{
	if (evaluate()) {
#ifdef DEBUG_WHY
 		std::cout << "   AstRoot::why evaluates returning\n";
#endif
		return false;
	}

	theReasonWhy = "expression ";
	theReasonWhy += expression(true); // provide additional state
	theReasonWhy += " does not evaluate";
#ifdef DEBUG_WHY
 	std::cout << "    AstRoot::why  reason = " << theReasonWhy << "\n";
#endif
	return true;
}

void AstRoot::setParentNode(Node* p)
{
	if (left_) left_->setParentNode(p);
	if (right_) right_->setParentNode(p);
}

////////////////////////////////////////////////////////////////////////////////////

void AstNot::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitNot(this);
}

AstNot* AstNot::clone() const
{
   AstNot* ast = new AstNot();
   if (left_) ast->addChild( left_->clone() );
   return ast;
}

std::ostream& AstNot::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# NOT evaluate(" << evaluate() << ")";
	if (right_) os << " # ERROR has right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstNot::print_flat( std::ostream& os,bool add_bracket) const {
   os << name_;
   if (left_) {
      if (add_bracket) os << "(";
      left_->print_flat(os,add_bracket);
      if (add_bracket) os << ")";
   }
}

std::string AstNot::expression(bool why) const
{
 	std::string ret =  "NOT ";
 	ret += left_->expression(why);
	return ret;
}
////////////////////////////////////////////////////////////////////////////////////

void AstPlus::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitPlus(this);
}

AstPlus* AstPlus::clone() const
{
   AstPlus* ast = new AstPlus();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstPlus::print( std::ostream& os ) const {
 	Indentor::indent( os ) << "# PLUS  value(" << value() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstPlus::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " + ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstPlus::expression(bool why) const
{
 	std::string ret;
	if (left_) ret += left_->expression(why);
	ret  += " + ";
	if (right_) ret += right_->expression(why);
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstMinus::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitMinus(this);
}

AstMinus* AstMinus::clone() const
{
   AstMinus* ast = new AstMinus();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstMinus::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# MINUS value(" << value() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstMinus::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " - ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstMinus::expression(bool why) const
{
 	std::string ret;
	if (left_) ret += left_->expression(why);
	ret  += " - ";
	if (right_) ret += right_->expression(why);
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstDivide::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitDivide(this);
}

AstDivide* AstDivide::clone() const
{
   AstDivide* ast = new AstDivide();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

bool AstDivide::check(std::string& error_msg) const
{
   if (right_ && right_->value() == 0) {
      error_msg = "Divide by zero in trigger expression";
      return false;
   }
   return true;
}

int AstDivide::value() const {
   if (right_->value() == 0) {
      log(Log::ERR,"Divide by zero in trigger/complete expression");
      return 0;
   }
   return (left_->value() / right_->value()) ;
}

std::ostream& AstDivide::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# DIVIDE value(" << value() << ")";
	if (!left_)  os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstDivide::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " / ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstDivide::expression(bool why) const
{
 	std::string ret;
	if (left_) ret += left_->expression(why);
	ret  += " / ";
	if (right_) ret += right_->expression(why);
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstMultiply::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitMultiply(this);
}

AstMultiply* AstMultiply::clone() const
{
   AstMultiply* ast = new AstMultiply();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstMultiply::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# MULTIPLY value(" << value() << ")";
	if (!left_)  os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstMultiply::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " * ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstMultiply::expression(bool why) const
{
 	std::string ret;
	if (left_) ret += left_->expression(why);
	ret  += " * ";
	if (right_) ret += right_->expression(why);
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstModulo::accept(ExprAstVisitor& v)
{
   AstRoot::accept(v);
   v.visitModulo(this);
}

AstModulo* AstModulo::clone() const
{
   AstModulo* ast = new AstModulo();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

bool AstModulo::check(std::string& error_msg) const
{
   if (right_ && right_->value() == 0) {
      error_msg = "Modulo by zero in trigger expression";
      return false;
   }
   return true;
}

int AstModulo::value() const
{
   if (right_->value() == 0) {
      log(Log::ERR,"Modulo by zero in trigger/complete expression");
      return 0;
   }
   return (left_->value() % right_->value());
}

std::ostream& AstModulo::print( std::ostream& os ) const {
   Indentor::indent( os ) << "# Modulo value(" << value() << ")";
   if (!left_) os << " # ERROR has no left_";
   if (!right_) os << " # ERROR has no right_";
   os << "\n";
   return AstRoot::print( os );
}

void AstModulo::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " % ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstModulo::expression(bool why) const
{
   std::string ret;
   if (left_) ret += left_->expression(why);
   ret  += " % ";
   if (right_) ret += right_->expression(why);
   return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstAnd::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitAnd(this);
}

AstAnd* AstAnd::clone() const
{
   AstAnd* ast = new AstAnd();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstAnd::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# AND evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstAnd::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " and ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstAnd::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " AND ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstOr::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitOr(this);
}

AstOr* AstOr::clone() const
{
   AstOr* ast = new AstOr();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstOr::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# OR evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstOr::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " or ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstOr::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " OR ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitEqual(this);
}

AstEqual* AstEqual::clone() const
{
   AstEqual* ast = new AstEqual();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstEqual::print( std::ostream& os ) const {
 	Indentor::indent( os ) << "# EQUAL   evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " == ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstEqual::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " == ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstNotEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitNotEqual(this);
}
AstNotEqual* AstNotEqual::clone() const
{
   AstNotEqual* ast = new AstNotEqual();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstNotEqual::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# NOT_EQUAL   evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}

void AstNotEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " != ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstNotEqual::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " != ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstLessEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitLessEqual(this);
}
AstLessEqual* AstLessEqual::clone() const
{
   AstLessEqual* ast = new AstLessEqual();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstLessEqual::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# LESS_EQUAL   evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}
void AstLessEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " <= ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}
std::string AstLessEqual::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " <= ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstGreaterEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitGreaterEqual(this);
}
AstGreaterEqual* AstGreaterEqual::clone() const
{
   AstGreaterEqual* ast = new AstGreaterEqual();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstGreaterEqual::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# GREATER_EQUAL   evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}
void AstGreaterEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " >= ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstGreaterEqual::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " >= ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstGreaterThan::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitGreaterThan(this);
}
AstGreaterThan* AstGreaterThan::clone() const
{
   AstGreaterThan* ast = new AstGreaterThan();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstGreaterThan::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# GREATER_THAN   evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}
void AstGreaterThan::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " > ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstGreaterThan::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " > ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstLessThan::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitLessThan(this);
}

AstLessThan* AstLessThan::clone() const
{
   AstLessThan* ast = new AstLessThan();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

std::ostream& AstLessThan::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# LESS_THAN   evaluate(" << evaluate() << ")";
	if (!left_) os << " # ERROR has no left_";
	if (!right_) os << " # ERROR has no right_";
	os << "\n";
	return AstRoot::print( os );
}
void AstLessThan::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " < ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}
std::string AstLessThan::expression(bool why) const
{
 	std::string ret("(");
	if (left_) ret += left_->expression(why);
	ret  += " < ";
	if (right_) ret += right_->expression(why);
	ret += ")";
 	return ret;
}

////////////////////////////////////////////////////////////////////////////////////

void AstLeaf::accept(ExprAstVisitor& v)
{
	v.visitLeaf(this);
}

///////////////////////////////////////////////////////////////////////////////////
void AstInteger::accept(ExprAstVisitor& v)
{
	v.visitInteger(this); // Not calling base
}

AstInteger* AstInteger::clone() const
{
   AstInteger* ast = new AstInteger(value_);
   return ast;
}

std::ostream& AstInteger::print( std::ostream& os ) const {
 	Indentor in;
	return Indentor::indent( os ) << "# LEAF_INTEGER " << value() << "\n";
}

void AstInteger::print_flat(std::ostream& os,bool /*add_bracket*/) const {
   os << value_;
}

std::string AstInteger::expression(bool /*why*/) const
{
	std::stringstream ss;
	ss << value();
	return ss.str();
}

//////////////////////////////////////////////////////////////////////////////////

void AstString::accept(ExprAstVisitor& v)
{
	v.visitString(this); // Not calling base
}

AstString* AstString::clone() const
{
   return new AstString(value_);
}

std::ostream& AstString::print( std::ostream& os ) const {
	Indentor in;
	return Indentor::indent( os ) << "# LEAF_STRING " << value_ << " value() = " << value() << "\n";
}
void AstString::print_flat(std::ostream& os,bool /*add_bracket*/) const {
   os << value_;
}

std::string AstString::expression(bool /*why*/) const
{
	return value_;
}

int AstString::value() const
{
	if (value_ == Event::SET()) {    // allow us to compare with a event and a string
		return 1;
	}
	if (value_ == Event::CLEAR()) {  // allow us to compare with a event and a string
		return 0;
	}
	// see if the value is convertible to a integer
	return Str::to_int( value_, 0/* value to return if conversion fails*/);
}

////////////////////////////////////////////////////////////////////////////////////

void AstNodeState::accept(ExprAstVisitor& v)
{
	v.visitNodeState(this);  // Not calling base
}

AstNodeState* AstNodeState::clone() const
{
   return new AstNodeState(state_);
}

std::ostream& AstNodeState::print( std::ostream& os ) const {
	Indentor in;
	return Indentor::indent( os ) << "# LEAF_NODE_STATE "
			<< DState::toString( state_ ) << "(" << value() << ")\n";
}

void AstNodeState::print_flat(std::ostream& os,bool /*add_bracket*/) const {
   os <<  DState::toString( state_ ) ;
}

std::string AstNodeState::expression(bool why) const
{
	return DState::toString(state_);
}

////////////////////////////////////////////////////////////////////////////////////

void AstEventState::accept(ExprAstVisitor& v)
{
	v.visitEventState(this);  // Not calling base
}

AstEventState* AstEventState::clone() const
{
   return new AstEventState(state_);
}

std::ostream& AstEventState::print( std::ostream& os ) const {
	Indentor in;
	return Indentor::indent( os ) << "# LEAF_EVENT_STATE " << state_ << "\n";
}

void AstEventState::print_flat(std::ostream& os,bool /*add_bracket*/) const {
   if (state_) os << Event::SET();
   else        os << Event::CLEAR();
}

std::string AstEventState::expression(bool /*why*/) const
{
	if (state_)  return Event::SET();
	return Event::CLEAR();
}

////////////////////////////////////////////////////////////////////////////////////

void AstNode::accept(ExprAstVisitor& v)
{
	v.visitNode(this);  // Not calling base
}

AstNode* AstNode::clone() const
{
   return new AstNode(nodePath_);
}

DState::State AstNode::state() const
{
   // This function is called hundreds of millions of times
   Node* refNode = referencedNode(); // call once, could be expensive
   if (refNode) return  refNode->dstate();
   return DState::UNKNOWN;
}

Node* AstNode::referencedNode() const
{
   // This function is called hundreds of millions of times
   // One of the server CPU **bottleneck's** is weak ptr locking
   // Note: gprof does not report on in-lined functions ?
   Node* ref =  get_ref_node();
   if ( ref )  {
      return ref;
   }
	if ( parentNode_ ) {
		std::string errorMsg;
      ref_node_ = parentNode_->findReferencedNode( nodePath_, errorMsg );
      return get_ref_node(); // can be NULL
	}
	return NULL;
}

Node* AstNode::referencedNode(std::string& errorMsg) const
{
   Node* ref =  get_ref_node();
   if ( ref )  {
      return ref;
   }
	if ( parentNode_ ) {
		ref_node_ = parentNode_->findReferencedNode( nodePath_, errorMsg );
		return get_ref_node(); // can be NULL
	}
	return NULL;
}

std::ostream& AstNode::print( std::ostream& os ) const {

 	Node* refNode = referencedNode(); // Only call once
	Indentor in;

	if ( refNode ) {
		Indentor::indent( os ) << "# LEAF_NODE node_(Found) nodePath_('" << nodePath_ << "') ";
		os << DState::toString(  refNode->dstate()  ) << "(" << static_cast<int>( refNode->dstate()) << ")\n";
	}
	else {
		Indentor::indent( os ) << "# LEAF_NODE node_(NULL) nodePath_('" << nodePath_ << "') ";
 		os << DState::toString( DState::UNKNOWN  ) << "(" << static_cast<int>(DState::UNKNOWN) << ")\n";
	}
	return os;
}

void AstNode::print_flat(std::ostream& os,bool /*add_bracket*/) const {
   os << nodePath_;
}

std::string AstNode::expression(bool why) const
{
	if (why) {
		Node* refNode = referencedNode(); // Only call once
		std::string ret = nodePath_;
		if ( refNode ) {
			ret += "(";
			ret += DState::toString(  refNode->dstate()  );
			ret += ")";
			return ret;
 		}
		else {
			ret += "(?";
			ret += DState::toString( DState::UNKNOWN  );
			ret += ")";
 		}
		return ret;
	}
	return  nodePath_;
}



////////////////////////////////////////////////////////////////////////////////////

void AstVariable::accept(ExprAstVisitor& v)
{
	v.visitVariable(this);  // Not calling base
}

AstVariable* AstVariable::clone() const
{
   return new AstVariable(nodePath_,name_);
}

int AstVariable::value() const
{
	VariableHelper varHelper(this);
	return varHelper.value();
}

int AstVariable::minus(Ast* right) const
{
   VariableHelper varHelper(this);
   return varHelper.minus(right->value());
}

int AstVariable::plus(Ast* right) const
{
   VariableHelper varHelper(this);
   return varHelper.plus(right->value());
}

std::ostream& AstVariable::print( std::ostream& os ) const
{
	VariableHelper varHelper(this);
	return varHelper.print(os);
}

void AstVariable::print_flat(std::ostream& os,bool /*add_bracket*/) const
{
   os << nodePath_ << Str::COLON() << name_;
}

std::string AstVariable::expression(bool why) const
{
	if (why) {
		VariableHelper varHelper(this);
		std::string ret = nodePath_;
		if ( !varHelper.theReferenceNode() )  ret += "(?)";
		ret += Str::COLON();
		ret += name_;
		ret += "(";

		std::string varType;
		int theValue;
		varHelper.varTypeAndValue(varType,theValue);

		std::stringstream ss; ss << "<type=" << varType << "> <value=" << theValue << ">";
		ret += ss.str();

		ret += ")";
		return ret;
	}
	return nodePath_ + Str::COLON() + name_;
}

Node* AstVariable::referencedNode() const
{
   Node* ref =  get_ref_node();
   if ( ref )  {
      return ref;
   }
   if ( parentNode_ ) {
		std::string ignoredErrorMsg;
      ref_node_ = parentNode_->findReferencedNode( nodePath_, name_, ignoredErrorMsg );
      return get_ref_node(); // can be NULL
   }
	return NULL;
}

Node* AstVariable::referencedNode(std::string& errorMsg) const
{
   Node* ref =  get_ref_node();
   if ( ref )  {
      return ref;
   }
	if ( parentNode_ ) {
		ref_node_ = parentNode_->findReferencedNode( nodePath_, name_, errorMsg );
		return get_ref_node(); // can be NULL
	}
	return NULL;
}


// ===============================================================================
// class VariableHelper:
// ===============================================================================
VariableHelper::VariableHelper(const AstVariable* astVariable)
: astVariable_(astVariable), theReferenceNode_(NULL)
{
	// For *this* constructor we don't care about errors'
	std::string errorMsg;
	theReferenceNode_ = astVariable_->referencedNode( errorMsg );
	if ( !theReferenceNode_ ) {
		// A node can be NULL  if :
		// 1/ parentNode is NOT set
		// 2/ when its a extern path. i.e corresponding suite not loaded yet
		return;
	}
	LOG_ASSERT(errorMsg.empty(),""); // when a reference node is found, the error msg should be empty
}

// ***NOTE*** This constructor is called during AST construction***. i.e AstResolveVisitor
// ********** It is used to report errors and to Flag whether meter or event is used
// ********** in a trigger expression for the simulator
VariableHelper::VariableHelper(const AstVariable* astVariable, std::string& errorMsg)
: astVariable_(astVariable), theReferenceNode_(NULL)
{
	// for *this* constructor we want to report errors
	theReferenceNode_ = astVariable_->referencedNode( errorMsg );
	if ( !theReferenceNode_ ) {
		// A node can be NULL  if :
		// 1/ parentNode is NOT set
		// 2/ when its a extern path. i.e corresponding suite not loaded yet
		return;
	}
	LOG_ASSERT(errorMsg.empty(),""); // when a reference node is found, the error msg should be empty

	// Find in order, event, meter, user variable, repeat, generated variable
	// ALSO: if meter or event mark as used in trigger, for simulator
 	if (theReferenceNode_->findExprVariable( astVariable_->name() ) ) {
		return;
	}

	std::stringstream ss;
	ss << "From expression Variable " << astVariable_->nodePath() << Str::COLON() << astVariable_->name() ;
	ss << " the referenced node is " << theReferenceNode_->debugNodePath() << "\n";
	errorMsg += ss.str();
	errorMsg += "Could not find event, meter, variable, repeat, or generated variable of name('";
	errorMsg += astVariable_->name();
	errorMsg += "') on node ";
	errorMsg += theReferenceNode_->debugNodePath();
	errorMsg += "\n";

	// FAILED to find astVar->name(), for node theReferenceNode on event, meter,
	// user variable, repeat, generated variable
	// SET theReferenceNode_ to NULL, since it does nor reference the Expression variable
	theReferenceNode_ = NULL;
}

int VariableHelper::value() const
{
	if ( theReferenceNode_ ) {
		return theReferenceNode_->findExprVariableValue(astVariable_->name());
	}
	return 0;
}

int VariableHelper::plus(int val) const
{
   if ( theReferenceNode_ ) {
      return theReferenceNode_->findExprVariableValueAndPlus(astVariable_->name(),val);
   }
   return val;
}

 int VariableHelper::minus(int val) const
 {
    if ( theReferenceNode_ ) {
       return theReferenceNode_->findExprVariableValueAndMinus(astVariable_->name(),val);
    }
    return -val;
 }

void VariableHelper::varTypeAndValue(std::string& varType, int & theValue) const
{
	if ( theReferenceNode_ ) {
		theValue = theReferenceNode_->findExprVariableValueAndType( astVariable_->name(), varType  );
		return;
	}
	varType = "variable-not-found";
 	theValue = 0;
}

std::ostream& VariableHelper::print( 	std::ostream& os ) const
{
	Indentor in;
	Indentor::indent( os ) << "# " << astVariable_->nodePath() << Str::COLON() << astVariable_->name();

	if ( theReferenceNode_ ) {
		os << " (";
		theReferenceNode_->findExprVariableAndPrint(astVariable_->name(), os);
		os << ")";
	}
	else {
	   os << " referencedNode(NULL) nodePath_('" << astVariable_->nodePath() << "') value(0)";
	}
	os << "\n";
 	return os;
}

std::ostream& operator<<( std::ostream& os, const Ast& d){return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstTop& d){return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstRoot& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstNot& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstPlus& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstMinus& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstDivide& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstMultiply& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstModulo& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstAnd& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstOr& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstEqual& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstNotEqual& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstLessEqual& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstGreaterEqual& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstGreaterThan& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstLessThan& d )    {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstLeaf& d )      {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstInteger& d)    {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstString& d)    {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstNodeState& d)  {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstEventState& d) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstNode& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstVariable& d ) {return d.print( os );}
