#ifndef EXPRESSION_HPP_
#define EXPRESSION_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #20 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <ostream>
#include <memory> // for unique_ptr

#include "Serialization.hpp"
#include "ExprAst.hpp"
class Node;

/// class PartExpression:
///  Hold a single expression, optional can specify whether it is to be And' or
///  'Ored' when used as a part of a larger expression.
///  Uses compiler , generated destructor, assignment,  copy constructor
class PartExpression  {
public:
   enum ExprType { FIRST, AND, OR };

   explicit PartExpression(const std::string& expression)
   : exp_(expression), exp_type_(FIRST) {}

   PartExpression(const std::string& expression, bool and_type)
   : exp_(expression), exp_type_( (and_type) ? AND : OR) {}

   PartExpression()
   : exp_type_(FIRST) {}

   const std::string& expression() const  { return exp_;}
   bool andExpr() const { return (exp_type_ == AND) ? true : false ;}
   bool orExpr() const  { return (exp_type_ == OR)  ? true : false ;}

   std::string toString(const std::string& exprType) const;
   std::ostream& print(std::ostream&,const std::string& exprType,bool isFree) const;

   bool operator==(const PartExpression& rhs) const {
      return exp_type_ == rhs.exp_type_ && exp_ == rhs.exp_;
   }
   bool operator!=(const PartExpression& rhs) const { return !operator==(rhs); }

   /// Parse the expression and create the abstract syntax tree
   std::unique_ptr<AstTop> parseExpressions(std::string& errorMsg) const;

   void set_expr_type(ExprType t) { exp_type_ = t;}
   ExprType expr_type() const { return exp_type_;}

private:
   std::string exp_;
   ExprType    exp_type_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(CEREAL_NVP(exp_),
         CEREAL_NVP(exp_type_));
   }
};

////////////////////////////////////////////////////////////////////////////////////////
// Class Expression:
//       A Expression occur in a Trigger or Complete statement
//       This class hold a number of part expression.
//       It can also create a  single AST from the part expressions
// NOTE: Distinguish between parser errors, and node path resolution
//       Here we are only concerned with parser errors.
// Use compiler , generated destructor, assignment,  copy constructor
class Expression  {
public:
   explicit Expression(const std::string& expression);
   explicit Expression(const PartExpression& );
   Expression();
   Expression(const Expression& rhs);

   bool operator==( const Expression& rhs) const{
      if (free_ != rhs.free_) return false;
      return vec_ == rhs.vec_;
   }
   bool operator!=( const Expression& rhs) const {
      return !operator==(rhs);
   }

   /// Helper function, will parse the expression and return the abstract syntax tree

   static std::unique_ptr<AstTop> parse(
            const std::string& expression_to_parse,
            const std::string& error_msg_context); // Will throw for parse errors
   static std::unique_ptr<AstTop> parse_no_throw(
            const std::string& expression_to_parse,
            std::string& error_msg_context);


   /// User should add "trigger" or "complete" at the start.
   /// The part expression's are combined and returned as a single string
   std::string expression() const { return compose_expression(vec_);}
   static std::string compose_expression(const std::vector<PartExpression>& vec);

   /// Need to pass in trigger tag, since expression may be split over multiple lines
   /// trigger    "a == complete"
   /// trigger -a "b == complete"
   std::ostream& print(std::ostream&,const std::string& exprType) const;

   /// Use when we want to add compose a large expression form a set of smaller ones
   void add( const PartExpression& t );
   void add_expr( const std::vector<PartExpression>& vec );

   // ==============================================================================================
   // CREATE AST tree for each expression and COMBINE AST for each expression into a single AST.
   // ==============================================================================================
   void createAST( Node* parent_node, const std::string& exprType, std::string& errorMsg ) const;
   AstTop* get_ast() const { return theCombinedAst_.get(); } // can return NULL

   /// Placed here rather than the expression tree. Since the expression
   /// tree is created on demand, and is not persisted
   void setFree();  // hence must be used before evaluate
   void clearFree(); // resets the free flag
   bool isFree() const { return free_;}
   bool empty() const { return vec_.empty();}
   const std::vector<PartExpression>& expr() const { return vec_;}

   // The state_change_no is never reset. Must be incremented if it can affect equality
   unsigned int state_change_no() const { return state_change_no_; }

private:  /// For use by python interface,
   friend void export_NodeAttr();
   friend class Trigger;
   friend class Complete;
   std::vector<PartExpression>::const_iterator part_begin() const { return vec_.begin();}
   std::vector<PartExpression>::const_iterator part_end() const   { return vec_.end();}

private:
   std::vector<PartExpression> vec_;
   bool                        free_;

   unsigned int state_change_no_;                    // *not* persisted, only used on server side

   // They are created on demand. reasons:
   // 1/ Help with AIX serialisation
   // 2/ Help to reduce network traffic
   mutable std::unique_ptr< AstTop >  theCombinedAst_; // *not* persisted, demand created

private:
   // prevent assignment since we have an unique_ptr
   Expression& operator=(Expression const& f);

private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
      ar(CEREAL_NVP(vec_),
         CEREAL_NVP(free_));
   }
};
#endif
