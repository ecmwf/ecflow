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
#include <cassert>
#include <iosfwd>
#include <boost/noncopyable.hpp>

#include "DState.hpp"
#include "Flag.hpp"
#include "NodeFwd.hpp"
namespace ecf { class ExprAstVisitor;} // forward declare class

//////////////////////////////////////////////////////////////////////////////////
class Ast {
public:
	Ast() = default;
	virtual ~Ast();

	virtual void accept(ecf::ExprAstVisitor&) = 0;
   virtual Ast* clone() const = 0;
   virtual bool is_attribute() const { return false; }
   virtual bool is_not() const { return false; }
   virtual bool isleaf() const { return false; }
	virtual bool isRoot() const { return false; }
   virtual AstTop* isTop() const { return nullptr; }
   virtual bool is_evaluateable() const { return false; }

	virtual void addChild(Ast*) {}
	virtual Ast* left() const { return nullptr;}
	virtual Ast* right() const { return nullptr;}
	virtual bool evaluate() const { assert(false); return false;}
	virtual bool empty() const { return true; }
   virtual int value() const { assert(false); return 0;} // only valid for leaf or operators
   virtual bool check(std::string& ) const { return true; } // check divide or modulo by zero

   virtual bool is_valid_ast(std::string& error_msg) const = 0;
   virtual std::ostream& print(std::ostream&) const = 0;
   virtual void print_flat(std::ostream&,bool add_brackets = false) const = 0;    // used for test
	virtual std::string type() const = 0;
	virtual void exprType(const std::string&) {}
	virtual std::string name() const { return expression(); } /* ABO */
   virtual std::string expression() const = 0;                      // recreate expression from AST
	virtual bool why(std::string& /*theReasonWhy*/,bool html = false) const;
   virtual std::string why_expression(bool html = false) const = 0; // recreate expression from AST for why command

	// Use for data arithmetic for REPEAT Date, Use default implementation for others
	// Currently *ONLY* works if repeat variable in on LHS
   virtual int minus(Ast* right) const { return (value() - right->value());}
   virtual int plus(Ast* right) const { return (value() + right->value());}

	virtual void setParentNode(Node*){} // traverse and set for interested nodes
	virtual void invalidate_trigger_references() const {}
};

class AstTop : public Ast {
public:
	AstTop()= default;
	~AstTop() override;

	void accept(ecf::ExprAstVisitor&) override;
   AstTop* clone() const override;

 	Ast* left() const override { return root_;}
 	void addChild(Ast* r) override { root_ = r;}
	AstTop* isTop() const override { return const_cast<AstTop*>(this); }
 	bool evaluate() const override;
   bool check(std::string& error_msg) const override;

	bool empty() const override { return (root_) ? false : true ; }
	std::ostream& print(std::ostream&) const override ;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;

	bool why(std::string& theReasonWhy,bool html = false) const override;
	std::string type() const override { return stype();}
	void exprType(const std::string& s) override { exprType_ = s;}
	static std::string stype() { return "top";}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	void setParentNode(Node*) override;
   void invalidate_trigger_references() const override;

private:
	Ast*        root_{nullptr};
	std::string exprType_; // trigger or complete
};

// This if one of AND, OR, == != <= >= +, -,*,!,%,/
class AstRoot : public Ast {
public:
   AstRoot()= default;
	~AstRoot() override;

 	bool isRoot() const override { return true;}
   bool is_evaluateable() const override { return true; }

   bool check(std::string& error_msg) const override;
	void accept(ecf::ExprAstVisitor&) override;
	void addChild(Ast* n) override;
 	Ast* left() const override { return left_;}
 	Ast* right() const override { return right_;}
	std::ostream& print(std::ostream& os) const override;
	bool empty() const override { return (left_ && right_) ? false : true ; }
	void setParentNode(Node*) override;

	virtual void set_root_name(const std::string&) {}
   void invalidate_trigger_references() const override;

protected:
   std::string do_why_expression(const std::string& root,bool html) const;
   std::string do_bracket_why_expression(const std::string& root,bool html) const;
   std::string do_false_bracket_why_expression(const std::string& root,bool html) const;

   std::string do_expression(const std::string& root ) const;
   std::string do_bracket_expression(const std::string& root ) const;

protected:
	Ast* left_{nullptr};
	Ast* right_{nullptr};
};

class AstNot : public AstRoot {
public:
	AstNot() : name_("! ") {}
	void accept(ecf::ExprAstVisitor&) override;
   AstNot* clone() const override;
   bool is_not() const override { return true;}

	bool evaluate() const override { assert(!right_);  return ! left_->evaluate();}
	int value() const override {  assert(!right_);     return ! left_->value();}
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
 	static std::string stype() { return "not";}
   void set_root_name(const std::string& n) override { name_ = n;}
private:
   std::string name_;
};


class AstPlus : public AstRoot {
public:
	AstPlus() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstPlus* clone() const override;

	bool evaluate() const override { return true;}
	int value() const override { return left_->plus(right_);}
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
 	static std::string stype() { return "plus";}
};

class AstMinus : public AstRoot {
public:
	AstMinus() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstMinus* clone() const override;

	bool evaluate() const override { return true;}
	int value() const override { return left_->minus(right_); }
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
 	static std::string stype() { return "minus";}
};

class AstDivide : public AstRoot {
public:
	AstDivide() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstDivide* clone() const override;
	bool evaluate() const override { return true;}
   bool check(std::string& error_msg) const override;
	int value() const override; // Log error if right hand side has value of zero
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
 	static std::string stype() { return "divide";}
};

class AstMultiply : public AstRoot {
public:
	AstMultiply() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstMultiply* clone() const override;
	bool evaluate() const override { return true;}
	int value() const override { return  (left_->value() * right_->value());}
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "multiply";}
};

class AstModulo : public AstRoot {
public:
   AstModulo()= default;
   void accept(ecf::ExprAstVisitor&) override;
   AstModulo* clone() const override;
   bool check(std::string& error_msg) const override;
   bool evaluate() const override { return true;}
   int value() const override; // Log error if right hand side has value of zero
   std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
   std::string type() const override { return stype();}
   std::string expression() const override;
   std::string why_expression(bool html = false) const override;
   static std::string stype() { return "modulo";}
};


class AstAnd : public AstRoot {
public:
	AstAnd() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstAnd* clone() const override;
	bool evaluate() const override { return (left_->evaluate() && right_->evaluate());}
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "and";}
};

class AstOr : public AstRoot {
public:
	AstOr() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstOr* clone() const override;
	bool evaluate() const override { return (left_->evaluate() || right_->evaluate());}
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "or";}
};

class AstEqual : public AstRoot {
public:
	AstEqual() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstEqual* clone() const override;
	bool evaluate() const override { return (left_->value() == right_->value()); }
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "equal";}
};

class AstNotEqual : public AstRoot {
public:
	AstNotEqual() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstNotEqual* clone() const override;
	bool evaluate() const override { return (left_->value() != right_->value()); }
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "not-equal";}
};

class AstLessEqual : public AstRoot {
public:
	AstLessEqual() = default;
	void accept(ecf::ExprAstVisitor&) override;
   AstLessEqual* clone() const override;
	bool evaluate() const override { return (left_->value() <= right_->value()); }
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "less-equal";}
};

class AstGreaterEqual : public AstRoot {
public:
	AstGreaterEqual() = default;
	bool evaluate() const override { return (left_->value() >= right_->value()); }
	void accept(ecf::ExprAstVisitor&) override;
   AstGreaterEqual* clone() const override;
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "greater-equal";}
};


class AstGreaterThan : public AstRoot {
public:
	AstGreaterThan() = default;

	bool evaluate() const override { return (left_->value() > right_->value()); }
	void accept(ecf::ExprAstVisitor&) override;
   AstGreaterThan* clone() const override;
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "greater-than";}
};


class AstLessThan : public AstRoot {
public:
	AstLessThan() = default;

	bool evaluate() const override { return (left_->value() < right_->value()); }
	void accept(ecf::ExprAstVisitor&) override;
   AstLessThan* clone() const override;
	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   bool is_valid_ast(std::string& error_msg) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "less-than";}
};

//=============================================================================================
/// class AstLeaf
/// represents Integer, String, Node State, event State, Node, variable
/// These always represent the right side of the tree
class AstLeaf : public Ast {
public:
  	AstLeaf() = default;
	void accept(ecf::ExprAstVisitor&) override;
	bool isleaf() const override { return true; }
   bool is_valid_ast(std::string&) const override { return true;}
};

class AstFunction : public AstLeaf {
public:
   enum FuncType { DATE_TO_JULIAN, JULIAN_TO_DATE };
   AstFunction(FuncType ft, Ast* arg) : ft_(ft), arg_(arg) { assert(arg_);}
   ~AstFunction() override { delete arg_;}

   bool is_evaluateable() const override { return true; }
   bool evaluate() const override { return value() != 0 ? true: false; }

   void accept(ecf::ExprAstVisitor&) override;
   AstFunction* clone() const override;
   int value() const override;
   std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   std::string type() const override { return stype();}
   std::string expression() const override;
   std::string why_expression(bool html = false) const override;
   static std::string stype() { return "AstFunction";}
   void setParentNode(Node* n) override;

   Ast* arg() const { return arg_;}
   FuncType ft() const { return ft_;}
private:
   FuncType ft_;
   Ast* arg_;
};


class AstInteger : public AstLeaf {
public:
	explicit AstInteger(int value) : value_(value) {}

   bool is_evaluateable() const override { return true; }
	bool evaluate() const override {  return value_; } // -1 -2 1 2 3 evaluates to true, 0 returns false

	void accept(ecf::ExprAstVisitor&) override;
   AstInteger* clone() const override;
 	int value() const override {  return value_;}
 	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "integer";}
private:
	int value_;
};


class AstNodeState : public AstLeaf {
public:
   explicit AstNodeState(DState::State s) : state_(s) {}

	void accept(ecf::ExprAstVisitor&) override;
   AstNodeState* clone() const override;
 	int value() const override {  return static_cast<int>(state_);}
 	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
	static std::string stype() { return "node-state";}
private:
	DState::State state_;
};

class AstEventState : public AstLeaf {
public:
   explicit AstEventState(bool b) : state_(b) {}

	void accept(ecf::ExprAstVisitor&) override;
   AstEventState* clone() const override;
 	int value() const override {  return state_;}
 	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
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
   explicit AstNode(const std::string& n) : parentNode_(nullptr), nodePath_(n) {}

	void accept(ecf::ExprAstVisitor&) override;
   AstNode* clone() const override;
 	int value() const override { return static_cast<int>(state());}
  	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
	std::string type() const override { return stype();}
   std::string expression() const override;
   std::string why_expression(bool html = false) const override;
 	void setParentNode(Node* n) override { parentNode_ = n; }
   void invalidate_trigger_references() const override { ref_node_.reset();}
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
   AstFlag(const std::string& n,ecf::Flag::Type ft) : flag_(ft),parentNode_(nullptr), nodePath_(n){}

   std::string name() const override;

   bool is_attribute() const override { return true; }
   // although AstFlag is leaf, However allow to evaluate to cope with
   //     ( ../family1/<flag>:late != 0 and ../family1/a:myEvent)
   // Treat this like an integer
   bool is_evaluateable() const override { return true; }
   bool evaluate() const override { return value() != 0 ? true: false; }

   void accept(ecf::ExprAstVisitor&) override;
   AstFlag* clone() const override;
   int value() const override;
   std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   std::string type() const override { return stype();}
   std::string expression() const override;
   std::string why_expression(bool html = false) const override;
   void setParentNode(Node* n) override { parentNode_ = n; }
   void invalidate_trigger_references() const override { ref_node_.reset();}
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
	: parentNode_(nullptr), nodePath_(nodePath), name_(variablename)  {}

	std::string name() const override { return name_;}
   bool is_attribute() const override { return true; }

	// although AstVariable is leaf, However allow to evaluate to cope with
   //     ( ../family1/a:myMeter >= 20 and ../family1/a:myEvent)
	// Treat this like an integer
   bool is_evaluateable() const override { return true; }
   bool evaluate() const override { return value() != 0 ? true: false; }

 	void accept(ecf::ExprAstVisitor&) override;
   AstVariable* clone() const override;
	int value() const override;
 	std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
	std::string type() const override { return stype();}
	std::string expression() const override;
   std::string why_expression(bool html = false) const override;
 	void setParentNode(Node* n) override { parentNode_ = n; }
   void invalidate_trigger_references() const override { ref_node_.reset();}

   int minus(Ast* right) const override;
   int plus(Ast* right) const override;

	Node* parentNode() const { return parentNode_; }
	Node* referencedNode() const;
	Node* referencedNode(std::string& errorMsg) const;

	static std::string stype() { return "variable";}
	const std::string& nodePath() const { return nodePath_;}

private:
	Node* get_ref_node() const { return ref_node_.lock().get(); }

	Node* parentNode_;
	std::string nodePath_;
	std::string name_;
	mutable weak_node_ptr ref_node_;
};

/// A variable: This can reference in the CURRENT order:
///     event,
///     meter,
///     user variable,
///     repeat  variable, for enumerated/string we use the positional value
///     generated variable
/// ** IT is treated in a same as an integer, and can appear in that context
//  ** i.e  "2 == (((:YMD / 100 ) % 100) % 3"
class AstParentVariable : public AstLeaf {
public:
   explicit AstParentVariable(const std::string& variablename)
   : parentNode_(nullptr), name_(variablename)  {}

   std::string name() const override { return name_;}
   bool is_attribute() const override { return true; }

   // although  AstParentVariable is leaf, However allow to evaluate to cope with
   //     ( :myMeter >= 20 and :myEvent)
   // Treat this like an integer
   bool is_evaluateable() const override { return true; }
   bool evaluate() const override { return value() != 0 ? true: false; }

   void accept(ecf::ExprAstVisitor&) override;
   AstParentVariable* clone() const override;
   int value() const override;
   std::ostream& print(std::ostream& os) const override;
   void print_flat(std::ostream&,bool add_brackets = false) const override;
   std::string type() const override { return stype();}
   std::string expression() const override;
   std::string why_expression(bool html = false) const override;
   void setParentNode(Node* n) override { parentNode_ = n; }
   void invalidate_trigger_references() const override { ref_node_.reset();}

   int minus(Ast* right) const override;
   int plus(Ast* right) const override;

   Node* parentNode() const { return parentNode_; }
   static std::string stype() { return "parent_variable";}

   Node* find_node_which_references_variable() const;
   Node* referencedNode() const { return find_node_which_references_variable();}

private:
   Node* parentNode_;
   std::string name_;
   mutable weak_node_ptr ref_node_;
};

// Helper class
class VariableHelper {
private:
  VariableHelper(const VariableHelper&) = delete;
  const VariableHelper& operator=(const VariableHelper&) = delete;
public:
   explicit VariableHelper(const AstVariable* astVariable);
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
