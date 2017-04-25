#ifndef EXPRAST_HPP_
#define EXPRAST_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #42 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

// The AST is now demand created, and hence we no longer need to persist it

#include <vector>
#include <assert.h>
#include <iostream>
#include <boost/noncopyable.hpp>

#include "DState.hpp"
#include "Flag.hpp"
#include "NodeFwd.hpp"
namespace ecf { class ExprAstVisitor;} // forward declare class

//////////////////////////////////////////////////////////////////////////////////
class Ast {
public:
	Ast() {}
	virtual ~Ast();

	virtual void accept(ecf::ExprAstVisitor&) = 0;
   virtual Ast* clone() const = 0;
   virtual bool is_attribute() const { return false; }
   virtual bool is_not() const { return false; }
   virtual bool isleaf() const { return false; }
	virtual bool isRoot() const { return false; }
   virtual AstTop* isTop() const { return NULL; }
   virtual bool is_evaluateable() const { return false; }

	virtual void addChild(Ast*) {}
	virtual Ast* left() const { return NULL;}
	virtual Ast* right() const { return NULL;}
	virtual bool evaluate() const { assert(false); return false;}
	virtual bool empty() const { return true; }
   virtual int value() const { assert(false); return 0;} // only valid for leaf or operators
   virtual bool check(std::string& ) const { return true; } // check divide or modulo by zero

   virtual bool is_valid_ast(std::string& error_msg) const = 0;
   virtual std::ostream& print(std::ostream&) const = 0;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const = 0;    // used for test
	virtual std::string type() const = 0;
	virtual void exprType(const std::string&) {}
	std::string name() { return expression(); } /* ABO */
   virtual std::string expression() const = 0;                      // recreate expression from AST
	virtual bool why(std::string& /*theReasonWhy*/,bool html = false) const;
   virtual std::string why_expression(bool html = false) const = 0; // recreate expression from AST for why command

	// Use for data arithmetic for REPEAT Date, Use default implementation for others
	// Currently *ONLY* works if repeat variable in on LHS
   virtual int minus(Ast* right) const { return (value() - right->value());}
   virtual int plus(Ast* right) const { return (value() + right->value());}

	virtual void setParentNode(Node*){} // traverse and set for interested nodes
};

class AstTop : public Ast {
public:
	AstTop() : root_(NULL) {}
	virtual ~AstTop();

	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstTop* clone() const;

 	virtual Ast* left() const { return root_;}
 	virtual void addChild(Ast* r) { root_ = r;}
	virtual AstTop* isTop() const { return const_cast<AstTop*>(this); }
 	virtual bool evaluate() const;
   virtual bool check(std::string& error_msg) const;

	virtual bool empty() const { return (root_) ? false : true ; }
	virtual std::ostream& print(std::ostream&) const ;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;

	virtual bool why(std::string& theReasonWhy,bool html = false) const;
	virtual std::string type() const { return stype();}
	virtual void exprType(const std::string& s) { exprType_ = s;}
	static std::string stype() { return "top";}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	virtual void setParentNode(Node*);

private:
	Ast*        root_;
	std::string exprType_; // trigger or complete
};

// This if one of AND, OR, == != <= >= +, -,*,!,%,/
class AstRoot : public Ast {
public:
   AstRoot() :left_(NULL), right_(NULL) {}
	virtual ~AstRoot();

 	virtual bool isRoot() const { return true;}
   virtual bool is_evaluateable() const { return true; }

   virtual bool check(std::string& error_msg) const;
	virtual void accept(ecf::ExprAstVisitor&);
	virtual void addChild(Ast* n);
 	virtual Ast* left() const { return left_;}
 	virtual Ast* right() const { return right_;}
	virtual std::ostream& print(std::ostream& os) const;
	virtual bool empty() const { return (left_ && right_) ? false : true ; }
	virtual void setParentNode(Node*);

	virtual void set_root_name(const std::string&) {}
protected:
   std::string do_why_expression(const std::string& root,bool html) const;
   std::string do_bracket_why_expression(const std::string& root,bool html) const;
   std::string do_false_bracket_why_expression(const std::string& root,bool html) const;

   std::string do_expression(const std::string& root ) const;
   std::string do_bracket_expression(const std::string& root ) const;

protected:
	Ast* left_;
	Ast* right_;
};

class AstNot : public AstRoot {
public:
	AstNot() : name_("! ") {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstNot* clone() const;
   virtual bool is_not() const { return true;}

	virtual bool evaluate() const { assert(!right_);  return ! left_->evaluate();}
	virtual int value() const {  assert(!right_);     return ! left_->value();}
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
 	static std::string stype() { return "not";}
   virtual void set_root_name(const std::string& n) { name_ = n;}
private:
   std::string name_;
};


class AstPlus : public AstRoot {
public:
	AstPlus() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstPlus* clone() const;

	virtual bool evaluate() const { return true;}
	virtual int value() const { return left_->plus(right_);}
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
 	static std::string stype() { return "plus";}
};

class AstMinus : public AstRoot {
public:
	AstMinus() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstMinus* clone() const;

	virtual bool evaluate() const { return true;}
	virtual int value() const { return left_->minus(right_); }
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
 	static std::string stype() { return "minus";}
};

class AstDivide : public AstRoot {
public:
	AstDivide() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstDivide* clone() const;
	virtual bool evaluate() const { return true;}
   virtual bool check(std::string& error_msg) const;
	virtual int value() const; // Log error if right hand side has value of zero
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
 	static std::string stype() { return "divide";}
};

class AstMultiply : public AstRoot {
public:
	AstMultiply() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstMultiply* clone() const;
	virtual bool evaluate() const { return true;}
	virtual int value() const { return  (left_->value() * right_->value());}
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "multiply";}
};

class AstModulo : public AstRoot {
public:
   AstModulo(){}
   virtual void accept(ecf::ExprAstVisitor&);
   virtual AstModulo* clone() const;
   virtual bool check(std::string& error_msg) const;
   virtual bool evaluate() const { return true;}
   virtual int value() const; // Log error if right hand side has value of zero
   virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
   virtual std::string type() const { return stype();}
   virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
   static std::string stype() { return "modulo";}
};


class AstAnd : public AstRoot {
public:
	AstAnd() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstAnd* clone() const;
	virtual bool evaluate() const { return (left_->evaluate() && right_->evaluate());}
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "and";}
};

class AstOr : public AstRoot {
public:
	AstOr() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstOr* clone() const;
	virtual bool evaluate() const { return (left_->evaluate() || right_->evaluate());}
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "or";}
};

class AstEqual : public AstRoot {
public:
	AstEqual() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstEqual* clone() const;
	virtual bool evaluate() const { return (left_->value() == right_->value()); }
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "equal";}
};

class AstNotEqual : public AstRoot {
public:
	AstNotEqual() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstNotEqual* clone() const;
	virtual bool evaluate() const { return (left_->value() != right_->value()); }
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "not-equal";}
};

class AstLessEqual : public AstRoot {
public:
	AstLessEqual() {}
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstLessEqual* clone() const;
	virtual bool evaluate() const { return (left_->value() <= right_->value()); }
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "less-equal";}
};

class AstGreaterEqual : public AstRoot {
public:
	AstGreaterEqual() {}
	virtual bool evaluate() const { return (left_->value() >= right_->value()); }
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstGreaterEqual* clone() const;
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "greater-equal";}
};


class AstGreaterThan : public AstRoot {
public:
	AstGreaterThan() {}

	virtual bool evaluate() const { return (left_->value() > right_->value()); }
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstGreaterThan* clone() const;
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "greater-than";}
};


class AstLessThan : public AstRoot {
public:
	AstLessThan() {}

	virtual bool evaluate() const { return (left_->value() < right_->value()); }
	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstLessThan* clone() const;
	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual bool is_valid_ast(std::string& error_msg) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "less-than";}
};

//=============================================================================================
/// class AstLeaf
/// represents Integer, String, Node State, event State, Node, variable
/// These always represent the right side of the tree
class AstLeaf : public Ast {
public:
  	AstLeaf() {}
	virtual void accept(ecf::ExprAstVisitor&);
	virtual bool isleaf() const { return true; }
   virtual bool is_valid_ast(std::string&) const { return true;}
};

class AstFunction : public AstLeaf {
public:
   enum FuncType { DATE_TO_JULIAN, JULIAN_TO_DATE };
   AstFunction(FuncType ft, Ast* arg) : ft_(ft), arg_(arg) { assert(arg_);}
   ~AstFunction() { delete arg_;}

   virtual bool is_evaluateable() const { return true; }
   virtual bool evaluate() const { return value() != 0 ? true: false; }

   virtual void accept(ecf::ExprAstVisitor&);
   virtual AstFunction* clone() const;
   virtual int value() const;
   virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual std::string type() const { return stype();}
   virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
   static std::string stype() { return "AstFunction";}
   virtual void setParentNode(Node* n);

   Ast* arg() const { return arg_;}
   FuncType ft() const { return ft_;}
private:
   FuncType ft_;
   Ast* arg_;
};


class AstInteger : public AstLeaf {
public:
	AstInteger(int value) : value_(value) {}

   virtual bool is_evaluateable() const { return true; }
	virtual bool evaluate() const {  return value_; } // -1 -2 1 2 3 evaluates to true, 0 returns false

	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstInteger* clone() const;
 	virtual int value() const {  return value_;}
 	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "integer";}
private:
	int value_;
};


class AstNodeState : public AstLeaf {
public:
	AstNodeState(DState::State s) : state_(s) {}

	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstNodeState* clone() const;
 	virtual int value() const {  return static_cast<int>(state_);}
 	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "node-state";}
private:
	DState::State state_;
};

class AstEventState : public AstLeaf {
public:
	AstEventState(bool b) : state_(b) {}

	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstEventState* clone() const;
 	virtual int value() const {  return state_;}
 	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
	static std::string stype() { return "event-state";}
private:
	bool state_;
};


/// This class will need to determine the corresponding node pointer
/// This is required so that during evaluation we don't need to search for the Node.
/// represent nodeName(a), dotPath(./a), dot dot path(../a/b)
///
/// A Node without a corresponding Node* will return the integer value of
/// DState::UNKNOWN for the value() function. This will allow for trigger
/// of the form:  trigger a == complete or a == unknown
/// to be evaluated.

class AstNode : public AstLeaf {
public:
	AstNode(const std::string& n) : parentNode_(NULL), nodePath_(n) {}

	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstNode* clone() const;
 	virtual int value() const { return static_cast<int>(state());}
  	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
	virtual std::string type() const { return stype();}
   virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
 	virtual void setParentNode(Node* n) { parentNode_ = n; }
	static std::string stype() { return "node";}

	const std::string& nodePath() const { return nodePath_;}
	Node* referencedNode() const;
	Node* referencedNode(std::string& errorMsg) const;
	Node* parentNode() const { return parentNode_; }
	DState::State state() const;

private:
	Node* get_ref_node() const { return ref_node_.lock().get(); }
 	Node* parentNode_;                 // should always be non null, before evaluate.
   std::string nodePath_;
 	mutable weak_node_ptr ref_node_;
};

class AstFlag : public AstLeaf {
public:
   AstFlag(const std::string& n,ecf::Flag::Type ft) : flag_(ft),parentNode_(NULL), nodePath_(n){}

   virtual bool is_attribute() const { return true; }
   // although AstFlag is leaf, However allow to evaluate to cope with
   //     ( ../family1/<flag>:late != 0 and ../family1/a:myEvent)
   // Treat this like an integer
   virtual bool is_evaluateable() const { return true; }
   virtual bool evaluate() const { return value() != 0 ? true: false; }

   virtual void accept(ecf::ExprAstVisitor&);
   virtual AstFlag* clone() const;
   virtual int value() const;
   virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
   virtual std::string type() const { return stype();}
   virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
   virtual void setParentNode(Node* n) { parentNode_ = n; }
   static std::string stype() { return "flag";}

   const std::string& nodePath() const { return nodePath_;}
   Node* referencedNode() const;
   Node* referencedNode(std::string& errorMsg) const;
   Node* parentNode() const { return parentNode_; }

private:
   Node* get_ref_node() const { return ref_node_.lock().get(); }
   ecf::Flag::Type flag_;
   Node* parentNode_;                 // should always be non null, before evaluate.
   std::string nodePath_;
   mutable weak_node_ptr ref_node_;
};


/// A variable: This can reference in the CURRENT order:
///     event,
///     meter,
///     user variable,
///     repeat  variable, for enumerated/string we use the positional value
///     generated variable
/// ** IT is treated in a same as an integer, and can appear in that context
//  ** i.e  "2 == (((/seasplots/lag:YMD / 100 ) % 100) % 3"
class AstVariable : public AstLeaf {
public:
	AstVariable(const std::string& nodePath, const std::string& variablename)
	: parentNode_(NULL), nodePath_(nodePath), name_(variablename)  {}

   virtual bool is_attribute() const { return true; }

	// although AstVariable is leaf, However allow to evaluate to cope with
   //     ( ../family1/a:myMeter >= 20 and ../family1/a:myEvent)
	// Treat this like an integer
   virtual bool is_evaluateable() const { return true; }
   virtual bool evaluate() const { return value() != 0 ? true: false; }

 	virtual void accept(ecf::ExprAstVisitor&);
   virtual AstVariable* clone() const;
	virtual int value() const;
 	virtual std::ostream& print(std::ostream& os) const;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const;
	virtual std::string type() const { return stype();}
	virtual std::string expression() const;
   virtual std::string why_expression(bool html = false) const;
 	virtual void setParentNode(Node* n) { parentNode_ = n; }

   virtual int minus(Ast* right) const;
   virtual int plus(Ast* right) const;

	Node* parentNode() const { return parentNode_; }
	Node* referencedNode() const;
	Node* referencedNode(std::string& errorMsg) const;

	static std::string stype() { return "variable";}
	const std::string& nodePath() const { return nodePath_;}
	const std::string& name() const { return name_;}

private:
	Node* get_ref_node() const { return ref_node_.lock().get(); }

	Node* parentNode_;
	std::string nodePath_;
	std::string name_;
	mutable weak_node_ptr ref_node_;
};

// Helper class
class VariableHelper : private boost::noncopyable {
public:
	VariableHelper(const AstVariable* astVariable);
	VariableHelper(const AstVariable* astVariable, std::string& errorMsg);

	int value() const;
   int plus(int)const;
   int minus(int)const;

 	std::ostream& print(std::ostream& os) const;
 	Node* theReferenceNode() const { return theReferenceNode_;}

 	void varTypeAndValue(std::string& varType, int & value) const;

private:
	const AstVariable* astVariable_;
 	Node* theReferenceNode_;
};


//17
std::ostream& operator<<(std::ostream& os, const Ast&);
std::ostream& operator<<(std::ostream& os, const AstTop&);
std::ostream& operator<<(std::ostream& os, const AstRoot&);
std::ostream& operator<<(std::ostream& os, const AstNot&);
std::ostream& operator<<(std::ostream& os, const AstPlus&);
std::ostream& operator<<(std::ostream& os, const AstMinus&);
std::ostream& operator<<(std::ostream& os, const AstMultiply&);
std::ostream& operator<<(std::ostream& os, const AstDivide&);
std::ostream& operator<<(std::ostream& os, const AstModulo&);
std::ostream& operator<<(std::ostream& os, const AstAnd&);
std::ostream& operator<<(std::ostream& os, const AstOr&);
std::ostream& operator<<(std::ostream& os, const AstEqual&);
std::ostream& operator<<(std::ostream& os, const AstNotEqual&);
std::ostream& operator<<(std::ostream& os, const AstLessEqual&);
std::ostream& operator<<(std::ostream& os, const AstGreaterEqual&);
std::ostream& operator<<(std::ostream& os, const AstGreaterThan&);
std::ostream& operator<<(std::ostream& os, const AstLessThan&);
std::ostream& operator<<(std::ostream& os, const AstLeaf&);
std::ostream& operator<<(std::ostream& os, const AstInteger&);
std::ostream& operator<<(std::ostream& os, const AstNodeState&);
std::ostream& operator<<(std::ostream& os, const AstEventState&);
std::ostream& operator<<(std::ostream& os, const AstNode&);
std::ostream& operator<<(std::ostream& os, const AstVariable&);
std::ostream& operator<<(std::ostream& os, const AstFlag&);

#endif
