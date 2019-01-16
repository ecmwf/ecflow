//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #57 $ 
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

#include <iostream>
#include <vector>
#include <boost/foreach.hpp>

#include "ExprAst.hpp"
#include "Indentor.hpp"
#include "ExprAstVisitor.hpp"
#include "Node.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "Str.hpp"
#include "Cal.hpp"

using namespace ecf;
using namespace std;

////////////////////////////////////////////////////////////////////////////////////

Ast::~Ast() = default;

//#define DEBUG_WHY 1

bool Ast::why(std::string& theReasonWhy,bool html) const
{
   if (evaluate()) {
#ifdef DEBUG_WHY
      std::cout << "   Ast::why evaluates returning\n";
#endif
      return false;
   }

   theReasonWhy = "expression ";
   theReasonWhy += why_expression(html); // provide additional state
   theReasonWhy += " is false";
#ifdef DEBUG_WHY
   std::cout << "    Ast::why  reason = " << theReasonWhy << "\n";
#endif
   return true;
}

////////////////////////////////////////////////////////////////////////////////////

AstTop::~AstTop() { delete root_;}

void AstTop::accept(ExprAstVisitor& v)
{
	v.visitTop(this);
	root_->accept(v);
}

AstTop* AstTop::clone() const
{
   auto* top = new AstTop();
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
	Indentor::indent(os) << "# AstTop\n";
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

bool AstTop::is_valid_ast(std::string& error_msg) const
{
   if (root_) {
      return root_->is_valid_ast(error_msg);
   }
   error_msg = "AstTop: Abstract syntax tree creation failed";
   return false;
}

bool AstTop::why(std::string& theReasonWhy,bool html) const
{
	if (evaluate()) {
#ifdef DEBUG_WHY
 		std::cout << "   AstTop::why evaluates returning\n";
#endif
		return false;
	}
	return root_->why(theReasonWhy,html);
}

std::string AstTop::expression() const
{
	std::string ret = exprType_;
	if (root_) {
		ret += " ";
		ret += root_->expression();
	}
	return ret;
}

std::string AstTop::why_expression(bool html) const
{
   std::string ret = exprType_;
   if (root_) {
      ret += " ";
      ret += root_->why_expression(html);
   }
   return ret;
}

void AstTop::setParentNode(Node* p)
{
	if (root_) root_->setParentNode(p);
}

void AstTop::invalidate_trigger_references() const
{
   if (root_) root_->invalidate_trigger_references();
}

//////////////////////////////////////////////////////////////////////////////////////

AstRoot::~AstRoot() {
	delete left_;
	delete right_;
	left_ = nullptr;
	right_ = nullptr;
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
   // left_ should not be NULL, but keep clang static analyser happy
   if (left_) {
      if (left_->isRoot()) {
         Indentor in;
         left_->print( os );
      }
      else left_->print( os );
   }

	if (right_) { // right_ is empty for Not
		if (right_->isRoot()) {
			Indentor in;
			right_->print( os );
		}
		else right_->print( os ); ;
	}
	return os;
}

std::string AstRoot::do_why_expression(const std::string& root,bool html) const
{
   std::string ret;
   if (left_) ret += left_->why_expression(html);
   ret  += root;
   if (right_) ret += right_->why_expression(html);
   return ret;
}

std::string AstRoot::do_bracket_why_expression(const std::string& root,bool html) const
{
   std::string ret = "(";
   ret += do_why_expression(root,html);
   ret += ")";
   return ret;
}

std::string AstRoot::do_false_bracket_why_expression(const std::string& root,bool html) const
{
   std::string ret;
   if (html) ret += "<false>";  // still need guard with html for ecflowview ?
   ret += do_bracket_why_expression(root,html);
   if (html) ret += "</false>";
   return ret;
}

std::string AstRoot::do_expression(const std::string& root ) const
{
   std::string ret;
   if (left_) ret += left_->expression();
   ret  += root;
   if (right_) ret += right_->expression();
   return ret;
}

std::string AstRoot::do_bracket_expression(const std::string& root ) const
{
   std::string ret = "(";
   ret += do_expression(root);
   ret += ")";
   return ret;
}

void AstRoot::setParentNode(Node* p)
{
	if (left_) left_->setParentNode(p);
	if (right_) right_->setParentNode(p);
}

void AstRoot::invalidate_trigger_references() const
{
   if (left_)  left_->invalidate_trigger_references();
   if (right_) right_->invalidate_trigger_references();
}

////////////////////////////////////////////////////////////////////////////////////

void AstNot::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitNot(this);
}

AstNot* AstNot::clone() const
{
   auto* ast = new AstNot();
   if (left_) ast->addChild( left_->clone() );
   return ast;
}

std::ostream& AstNot::print( std::ostream& os ) const {
	Indentor::indent( os ) << "# NOT evaluate(" << evaluate() << ")";
	if (right_) os << " # ERROR has right_";
	os << "\n";
	return AstRoot::print( os );
}

bool AstNot::is_valid_ast(std::string& error_msg) const
{
   if (right_) {
      error_msg = "AstNot: should only have a single root";
      return false;
   }
   if (!left_) {
      error_msg = "AstNot: Does not have root";
      return false;
   }
   return left_->is_valid_ast(error_msg);
}

void AstNot::print_flat( std::ostream& os,bool add_bracket) const {
   os << name_;
   if (left_) {
      if (add_bracket) os << "(";
      left_->print_flat(os,add_bracket);
      if (add_bracket) os << ")";
   }
}

std::string AstNot::expression() const
{
 	std::string ret =  "NOT ";
 	ret += left_->expression();
	return ret;
}

std::string AstNot::why_expression(bool html) const
{
   if (evaluate()) return "true";

   std::string ret;
   if (html) ret += "<false>";
   ret += "not ";
   ret += left_->why_expression(html);
   if (html) ret += "</false>";
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
   auto* ast = new AstPlus();
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

bool AstPlus::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstPlus: has no left part";  return false;}
   if (!right_) { error_msg = "AstPlus: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstPlus::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " + ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstPlus::expression() const
{
   return do_expression(" + ");
}

std::string AstPlus::why_expression(bool html) const
{
   return do_why_expression(" + ",html);
}

////////////////////////////////////////////////////////////////////////////////////

void AstMinus::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitMinus(this);
}

AstMinus* AstMinus::clone() const
{
   auto* ast = new AstMinus();
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

bool AstMinus::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstMinus: has no left part"; return false;}
   if (!right_) { error_msg = "AstMinus: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstMinus::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " - ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstMinus::expression() const
{
   return do_expression(" - ");
}

std::string AstMinus::why_expression(bool html) const
{
   return do_why_expression(" - ",html);
}

////////////////////////////////////////////////////////////////////////////////////

void AstDivide::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitDivide(this);
}

AstDivide* AstDivide::clone() const
{
   auto* ast = new AstDivide();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

bool AstDivide::check(std::string& error_msg) const
{
   if (right_ && right_->value() == 0) {
      error_msg += " Error: Divide by zero in trigger expression";
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

bool AstDivide::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstDivide: has no left part"; return false;}
   if (!right_) { error_msg = "AstDivide: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstDivide::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " / ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstDivide::expression() const
{
   return do_expression(" / ");
}

std::string AstDivide::why_expression(bool html) const
{
   return do_why_expression(" / ",html);
}

////////////////////////////////////////////////////////////////////////////////////

void AstMultiply::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitMultiply(this);
}

AstMultiply* AstMultiply::clone() const
{
   auto* ast = new AstMultiply();
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

bool AstMultiply::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstMultiply: has no left part"; return false;}
   if (!right_) { error_msg = "AstMultiply: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstMultiply::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " * ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstMultiply::expression() const
{
   return do_expression(" * ");
}

std::string AstMultiply::why_expression(bool html) const
{
   return do_why_expression(" * ",html);
}

////////////////////////////////////////////////////////////////////////////////////

void AstModulo::accept(ExprAstVisitor& v)
{
   AstRoot::accept(v);
   v.visitModulo(this);
}

AstModulo* AstModulo::clone() const
{
   auto* ast = new AstModulo();
   if (left_) ast->addChild( left_->clone() );
   if (right_) ast->addChild( right_->clone() );
   return ast;
}

bool AstModulo::check(std::string& error_msg) const
{
   if (right_ && right_->value() == 0) {
      error_msg += " Error: Modulo by zero in trigger expression";
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

bool AstModulo::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstModulo: has no left part"; return false;}
   if (!right_) { error_msg = "AstModulo: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstModulo::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " % ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstModulo::expression() const
{
   return do_expression(" % ");
}

std::string AstModulo::why_expression(bool html) const
{
   return do_why_expression(" % ",html);
}

////////////////////////////////////////////////////////////////////////////////////

void AstAnd::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitAnd(this);
}

AstAnd* AstAnd::clone() const
{
   auto* ast = new AstAnd();
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

bool AstAnd::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstAnd: has no left part"; return false;}
   if (!right_) { error_msg = "AstAnd: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstAnd::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " and ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstAnd::expression() const
{
   return do_bracket_expression(" AND ");
}

std::string AstAnd::why_expression(bool html) const
{
   if (evaluate()) return "true";
   return do_false_bracket_why_expression(" and ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstOr::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitOr(this);
}

AstOr* AstOr::clone() const
{
   auto* ast = new AstOr();
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

bool AstOr::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstOr: has no left part"; return false;}
   if (!right_) { error_msg = "AstOr: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstOr::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " or ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstOr::expression() const
{
   return do_bracket_expression(" OR ");
}

std::string AstOr::why_expression(bool html) const
{
   if (evaluate()) return "true";
   return do_false_bracket_why_expression(" or ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitEqual(this);
}

AstEqual* AstEqual::clone() const
{
   auto* ast = new AstEqual();
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

bool AstEqual::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstEqual: has no left part"; return false;}
   if (!right_) { error_msg = "AstEqual: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " == ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstEqual::expression() const
{
   return do_bracket_expression(" == ");
}

std::string AstEqual::why_expression(bool html) const
{
   if (evaluate()) return "true";
   return do_false_bracket_why_expression(" == ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstNotEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitNotEqual(this);
}
AstNotEqual* AstNotEqual::clone() const
{
   auto* ast = new AstNotEqual();
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

bool AstNotEqual::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstNotEqual: has no left part"; return false;}
   if (!right_) { error_msg = "AstNotEqual: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstNotEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " != ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstNotEqual::expression() const
{
   return do_bracket_expression(" != ");
}

std::string AstNotEqual::why_expression(bool html) const
{
   if (evaluate()) return "true";
   return do_false_bracket_why_expression(" != ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstLessEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitLessEqual(this);
}
AstLessEqual* AstLessEqual::clone() const
{
   auto* ast = new AstLessEqual();
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

bool AstLessEqual::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstLessEqual: has no left part"; return false;}
   if (!right_) { error_msg = "AstLessEqual: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstLessEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " <= ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstLessEqual::expression() const
{
   return do_bracket_expression(" <= ");
}

std::string AstLessEqual::why_expression(bool html) const
{
   if (evaluate()) return "true";
   return do_false_bracket_why_expression(" <= ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstGreaterEqual::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitGreaterEqual(this);
}
AstGreaterEqual* AstGreaterEqual::clone() const
{
   auto* ast = new AstGreaterEqual();
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

bool AstGreaterEqual::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstGreaterEqual: has no left part"; return false;}
   if (!right_) { error_msg = "AstGreaterEqual: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstGreaterEqual::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " >= ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstGreaterEqual::expression() const
{
   return do_bracket_expression(" >= ");
}

std::string AstGreaterEqual::why_expression(bool html) const
{
   if (evaluate()) return "true";
   return do_false_bracket_why_expression(" >= ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstGreaterThan::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitGreaterThan(this);
}
AstGreaterThan* AstGreaterThan::clone() const
{
   auto* ast = new AstGreaterThan();
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

bool AstGreaterThan::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstGreaterThan: has no left part"; return false;}
   if (!right_) { error_msg = "AstGreaterThan: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstGreaterThan::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " > ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstGreaterThan::expression() const
{
   return do_bracket_expression(" > ");
}

std::string AstGreaterThan::why_expression(bool html) const
{
   if (evaluate()) return "true";
   return do_false_bracket_why_expression(" > ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstLessThan::accept(ExprAstVisitor& v)
{
	AstRoot::accept(v);
	v.visitLessThan(this);
}

AstLessThan* AstLessThan::clone() const
{
   auto* ast = new AstLessThan();
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

bool AstLessThan::is_valid_ast(std::string& error_msg) const
{
   if (!left_)  { error_msg = "AstLessThan: has no left part"; return false;}
   if (!right_) { error_msg = "AstLessThan: has no right part"; return false;}
   if ( left_->is_valid_ast(error_msg) && right_->is_valid_ast(error_msg) ) {
      return true;
   }
   return false;
}

void AstLessThan::print_flat(std::ostream& os,bool add_bracket) const {
   if (add_bracket) os << "(";
   if (left_) left_->print_flat(os,add_bracket);
   os << " < ";
   if (right_) right_->print_flat(os,add_bracket);
   if (add_bracket) os << ")";
}

std::string AstLessThan::expression() const
{
   return do_bracket_expression(" < ");
}

std::string AstLessThan::why_expression(bool html) const
{
   if (evaluate()) return "true";
   // Using ' < ' seems to mess up html
   return do_false_bracket_why_expression(" lt ",html); // false - allows GUI to colour the false expression part
}

////////////////////////////////////////////////////////////////////////////////////

void AstLeaf::accept(ExprAstVisitor& v)
{
	v.visitLeaf(this);
}

///////////////////////////////////////////////////////////////////////////////////

void AstFunction::accept(ecf::ExprAstVisitor& v)
{
   v.visitFunction(this); // Not calling base
}

AstFunction* AstFunction::clone() const
{
   return new AstFunction( ft_, arg()->clone() );
}

static int digits_in_integer(int number)
{
    int digits = 0;
    if (number < 0) digits = 1; // remove this line if '-' counts as a digit
    while (number) {
        number /= 10;
        digits++;
    }
    return digits;
}

int AstFunction::value() const {
   int arg_eval = arg_->value();

   switch (ft_) {
      case AstFunction::DATE_TO_JULIAN: {
         // cope with 8 digit, yyyymmdd and 10 digit yyyymmddhh
         int integer_digits = digits_in_integer(arg_eval);
         if (integer_digits == 10  ) arg_eval = arg_eval/100 ;
         else if (integer_digits != 8 ) return 0;
         return Cal::date_to_julian( arg_eval );}
      case AstFunction::JULIAN_TO_DATE:
         return Cal::julian_to_date( arg_eval );
      default: assert(false);
   }
   return 0;
}

std::ostream& AstFunction::print(std::ostream& os) const {
   Indentor in;
   switch (ft_) {
      case AstFunction::DATE_TO_JULIAN: return Indentor::indent( os ) << "# DATE_TO_JULIAN " << value() << "\n";
      case AstFunction::JULIAN_TO_DATE: return Indentor::indent( os ) << "# JULIAN_TO_DATE " << value() << "\n";
      default: assert(false);
   }
   return os;
}

void AstFunction::print_flat(std::ostream& os,bool add_brackets) const {
   switch (ft_) {
      case AstFunction::DATE_TO_JULIAN: os << "date_to_julian(arg:" << arg_->value() << ") = " << value(); break;
      case AstFunction::JULIAN_TO_DATE: os << "julian_to_date(arg:" << arg_->value() << ") = " << value(); break;
      default: assert(false);
   }
}

std::string AstFunction::expression() const
{
   std::stringstream ss;
   switch (ft_) {
      case AstFunction::DATE_TO_JULIAN: ss << "date_to_julian( arg:" << arg_->expression() << ") = " << value(); break;
      case AstFunction::JULIAN_TO_DATE: ss << "julian_to_date( arg:" << arg_->expression() << ") = " << value(); break;
      default: assert(false);
   }
   return ss.str();
}

std::string AstFunction::why_expression(bool html) const
{
   std::stringstream ss;
   switch (ft_) {
      case AstFunction::DATE_TO_JULIAN: ss << "date_to_julian( arg:" << arg_->why_expression(html) << ") = " << value(); break;
      case AstFunction::JULIAN_TO_DATE: ss << "julian_to_date( arg:" << arg_->why_expression(html) << ") = " << value(); break;
      default: assert(false);
   }
   return ss.str();
}

void AstFunction::setParentNode(Node* n)
{
   if (arg_) arg_->setParentNode(n);
}

///////////////////////////////////////////////////////////////////////////////////
void AstInteger::accept(ExprAstVisitor& v)
{
	v.visitInteger(this); // Not calling base
}

AstInteger* AstInteger::clone() const
{
   auto* ast = new AstInteger(value_);
   return ast;
}

std::ostream& AstInteger::print( std::ostream& os ) const {
 	Indentor in;
	return Indentor::indent( os ) << "# LEAF_INTEGER " << value() << "\n";
}

void AstInteger::print_flat(std::ostream& os,bool /*add_bracket*/) const {
   os << value_;
}

std::string AstInteger::expression() const
{
	std::stringstream ss;
	ss << value();
	return ss.str();
}

std::string AstInteger::why_expression(bool /*html*/) const
{
   return expression();
}

//////////////////////////////////////////////////////////////////////////////////

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

std::string AstNodeState::expression() const
{
	return DState::toString(state_);
}

std::string AstNodeState::why_expression(bool html) const
{
   if (html) return DState::to_html(state_);
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

std::string AstEventState::expression() const
{
	if (state_)  return Event::SET();
	return Event::CLEAR();
}

std::string AstEventState::why_expression(bool /*html*/) const
{
   return expression();
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
	return nullptr;
}

Node* AstNode::referencedNode(std::string& errorMsg) const
{
   Node* ref = get_ref_node();
   if ( ref )  {
      return ref;
   }
	if ( parentNode_ ) {
		ref_node_ = parentNode_->findReferencedNode( nodePath_, errorMsg );
		return get_ref_node(); // can be NULL
	}
	return nullptr;
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

std::string AstNode::expression() const
{
	return  nodePath_;
}

std::string AstNode::why_expression(bool html) const
{
   Node* refNode = referencedNode(); // Only call once
   std::string ret;
   if (html) {
      if (refNode) ret = Node::path_href_attribute(refNode->absNodePath(),nodePath_);
      else ret = Node::path_href_attribute(nodePath_);
   }
   else ret = nodePath_;

   if ( refNode ) {
      ret += "(";
      if (html) ret += DState::to_html(  refNode->dstate());
      else      ret += DState::toString( refNode->dstate());
      ret += ")";
      return ret;
   }
   else {
      ret += "?(";
      if (html) ret += DState::to_html(  DState::UNKNOWN);
      else      ret += DState::toString( DState::UNKNOWN);
      ret += ")";
   }
   return ret;
}

////////////////////////////////////////////////////////////////////////////////////

std::string AstFlag::name() const
{
   return ecf::Flag::enum_to_string( flag_ );
}

void AstFlag::accept(ExprAstVisitor& v)
{
   v.visitFlag(this);  // Not calling base
}

AstFlag* AstFlag::clone() const
{
   return new AstFlag(nodePath_,flag_);
}

int AstFlag::value() const
{
   Node* node = referencedNode();
   if (node && node->flag().is_set(flag_)) return 1;
   if (parentNode_ && nodePath_ == "/") {
      Defs* the_defs = parentNode_->defs();
      if (the_defs && the_defs->flag().is_set(flag_)) return 1;
   }
   return 0;
}

Node* AstFlag::referencedNode() const
{
   Node* ref = get_ref_node();
   if ( ref )  {
      return ref;
   }
   if ( parentNode_ ) {
      if (nodePath_ == "/") return nullptr; // reference to defs
      std::string errorMsg;
      ref_node_ = parentNode_->findReferencedNode( nodePath_, errorMsg );
      return get_ref_node(); // can be NULL
   }
   return nullptr;
}

Node* AstFlag::referencedNode(std::string& errorMsg) const
{
   Node* ref = get_ref_node();
   if ( ref )  {
      return ref;
   }
   if ( parentNode_ ) {
      if (nodePath_ == "/") return nullptr; // reference to defs
      ref_node_ = parentNode_->findReferencedNode( nodePath_, ecf::Flag::enum_to_string( flag_ ), errorMsg );
      return get_ref_node(); // can be NULL
   }
   return nullptr;
}

std::ostream& AstFlag::print( std::ostream& os ) const {

   Node* refNode = referencedNode(); // Only call once
   Indentor in;

   if ( refNode ) {
      Indentor::indent( os ) << "# LEAF_FLAG_NODE node_(Found) nodePath_('" << nodePath_ << "') ";
      os << ecf::Flag::enum_to_string( flag_ ) << "(" << static_cast<int>( refNode->flag().is_set(flag_)) << ")\n";
   }
   else {
      Indentor::indent( os ) << "# LEAF_FLAG_NODE node_(NULL) nodePath_('" << nodePath_ << "') ";
      os << ecf::Flag::enum_to_string( flag_ ) << "(0)\n";
   }
   return os;
}

void AstFlag::print_flat(std::ostream& os,bool /*add_bracket*/) const {
   os << expression();
}

std::string AstFlag::expression() const
{
   string ret = nodePath_;
   ret += "<flag>";
   ret += ecf::Flag::enum_to_string( flag_ );
   return ret;
}

std::string AstFlag::why_expression(bool html) const
{
   if (evaluate()) {
#ifdef DEBUG_WHY
      cout << " AstFlag::why_expression evaluates returning\n";
#endif
      return "true";
   }

#ifdef DEBUG_WHY
   cout << " AstFlag::why_expression html " << html << "\n";
#endif

   Node* ref_node = referencedNode(); // Only call once
   std::string ret;
   if (html) {
      // ecflow_ui expects: [attribute_type]attribute_path:attribute_name(value)
      // i.e                [limit]/suite/family/task:my_limit(value)
      // i.e                [flag]/suite/family/task:late(value)
      std::stringstream display_ss; display_ss << "[flag:" << ecf::Flag::enum_to_string(flag_) << "]" << nodePath_ ;
      std::string display_str = display_ss.str();

      std::string ref_str;
      if (ref_node) {
         std::stringstream ref_ss; ref_ss << "[flag:" << ecf::Flag::enum_to_string(flag_) << "]" << ref_node->absNodePath();
         ref_str = ref_ss.str();
      }
      else ref_str = display_str;

      ret = Node::path_href_attribute(ref_str,display_str);
      if ( !ref_node )  ret += "(?)";
      else {
         ret += "(";
         ret += boost::lexical_cast<std::string>(ref_node->flag().is_set(flag_) );
         ret += ")";
      }
   }
   else {
      ret = nodePath_;
      if ( !ref_node )  ret += "(?)";
      ret += "<flag>";
      ret += ecf::Flag::enum_to_string( flag_ );
      if (!ref_node) ret += "(?)";
      else {
         ret += "(";
         std::stringstream ss; ss << ref_node->flag().is_set(flag_) ;
         ret += ss.str();
         ret += ")";
      }
   }
   return ret;
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

std::string AstVariable::expression() const
{
	return nodePath_ + Str::COLON() + name_;
}

std::string AstVariable::why_expression(bool html) const
{
   VariableHelper varHelper(this);
   std::string ret;
   std::string varType;
   int theValue=0;
   varHelper.varTypeAndValue(varType,theValue);
   Node* ref_node = varHelper.theReferenceNode();

   if (html) {
      // ecflow_ui expects: [attribute_type]attribute_path:attribute_name
      // i.e                [limit]/suite/family/task:my_limit
      std::stringstream display_ss; display_ss << "[" << varType << "]" << nodePath_ << ":" << name_;
      std::string display_str = display_ss.str();
      std::string ref_str;
      if (ref_node) {
         std::stringstream ref_ss; ref_ss << "[" << varType << "]" << ref_node->absNodePath() << ":" << name_;
         ref_str = ref_ss.str();
      }
      else ref_str = display_str;

      ret = Node::path_href_attribute(ref_str,display_str);
      if ( !ref_node )  ret += "(?)";
      ret += "(";
      ret += boost::lexical_cast<std::string>(theValue);
      ret += ")";
   }
   else {
      ret = nodePath_;
      if ( !ref_node )  ret += "(?)";
      ret += Str::COLON();
      ret += name_;
      ret += "(";
      std::stringstream ss; ss << "type:" << varType << " value:" << theValue;
      ret += ss.str();
      ret += ")";
   }
   return ret;
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
	return nullptr;
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
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////

Node* AstParentVariable::find_node_which_references_variable() const
{
   Node* parent = parentNode_;
   while (parent) {
      if ( parent->findExprVariable(name_)) return parent;
      parent = parent->parent();
   }
   return nullptr;
}

void AstParentVariable::accept(ExprAstVisitor& v)
{
   v.visitParentVariable(this);  // Not calling base
}

AstParentVariable* AstParentVariable::clone() const
{
   return new AstParentVariable(name_);
}

int AstParentVariable::value() const
{
   Node* ref_node = find_node_which_references_variable();
   if (ref_node) {
      return ref_node->findExprVariableValue(name_);
   }
   return 0;
}

int AstParentVariable::minus(Ast* right) const
{
   Node* ref_node = find_node_which_references_variable();
   if (ref_node) {
      return ref_node->findExprVariableValueAndMinus(name_,right->value());
   }
   return right->value();
}

int AstParentVariable::plus(Ast* right) const
{
   Node* ref_node = find_node_which_references_variable();
   if (ref_node) {
      return ref_node->findExprVariableValueAndPlus(name_,right->value());
   }
   return right->value();
}

std::ostream& AstParentVariable::print( std::ostream& os ) const
{
   Indentor in;
   Indentor::indent( os ) << "# " << Str::COLON() << name_;

   Node* ref_node = find_node_which_references_variable();
   if (ref_node) {
      os << " (";
      ref_node->findExprVariableAndPrint(name_, os);
      os << ")";
      os << "\n";
      return os;
   }
   os << " referencedNode(NULL) value(0)";
   os << "\n";
   return os;
}

void AstParentVariable::print_flat(std::ostream& os,bool /*add_bracket*/) const
{
   os << Str::COLON() << name_;
}

std::string AstParentVariable::expression() const
{
   return Str::COLON() + name_;
}

std::string AstParentVariable::why_expression(bool html) const
{
   std::string varType = "variable-not-found";
   int theValue=0;
   std::string ret;
   Node* ref_node = find_node_which_references_variable();
   if (ref_node) {
      theValue = ref_node ->findExprVariableValueAndType( name_, varType );
   }
   if (html) {
      // ecflow_ui expects: [attribute_type]attribute_path:attribute_name
      // i.e                [limit]/suite/family/task:my_limit
      std::stringstream display_ss; display_ss << "[" << varType << "]" << ":" << name_;
      std::string display_str = display_ss.str();
      std::string ref_str;
      if (ref_node) {
         std::stringstream ref_ss; ref_ss << "[" << varType << "]" << ref_node->absNodePath() << ":" << name_;
         ref_str = ref_ss.str();
      }
      else ref_str = display_str;

      ret = Node::path_href_attribute(ref_str,display_str);
      if ( !ref_node )  ret += "(?)";
      ret += "(";
      ret += boost::lexical_cast<std::string>(theValue);
      ret += ")";
   }
   else {
      if ( !ref_node )  ret += "(?)";
      ret += Str::COLON();
      ret += name_;
      ret += "(";
      std::stringstream ss; ss << "type:" << varType << " value:" << theValue;
      ret += ss.str();
      ret += ")";
   }
   return ret;
}

// ===============================================================================
// class VariableHelper:
// ===============================================================================
VariableHelper::VariableHelper(const AstVariable* astVariable)
: astVariable_(astVariable), theReferenceNode_(nullptr)
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
: astVariable_(astVariable), theReferenceNode_(nullptr)
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

	// Find in order, event, meter, user variable, repeat, generated variable, limit
	// ALSO: if meter or event mark as used in trigger, for simulator
 	if (theReferenceNode_->findExprVariable( astVariable_->name() ) ) {
		return;
	}

	std::stringstream ss;
	ss << "From expression Variable " << astVariable_->nodePath() << Str::COLON() << astVariable_->name() ;
	ss << " the referenced node is " << theReferenceNode_->debugNodePath() << "\n";
	errorMsg += ss.str();
	errorMsg += "Could not find event, meter, variable, repeat, generated variable, limit or queue of name('";
	errorMsg += astVariable_->name();
	errorMsg += "') on node ";
	errorMsg += theReferenceNode_->debugNodePath();
	errorMsg += "\n";

	// FAILED to find astVar->name(), for node theReferenceNode on event, meter,
	// user variable, repeat, generated variable
	// SET theReferenceNode_ to NULL, since it does nor reference the Expression variable
	theReferenceNode_ = nullptr;
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
std::ostream& operator<<( std::ostream& os, const AstNodeState& d)  {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstEventState& d) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstNode& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstVariable& d ) {return d.print( os );}
std::ostream& operator<<( std::ostream& os, const AstFlag& d ) {return d.print( os );}
