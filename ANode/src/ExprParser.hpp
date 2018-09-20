#ifndef EXPRPARSER_HPP_
#define EXPRPARSER_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
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

#include <string>
#include <memory> // for unique_ptr
#include "ExprAst.hpp"


/// This class will parse a expression and create the abstract syntax tree
/// It will own the AST unless specifically released calling ast();
class ExprParser {
private:
  ExprParser(const ExprParser&) = delete;
  const ExprParser& operator=(const ExprParser&) = delete;
public:
   explicit ExprParser(const std::string& expression);

	/// Parse the expression, return true if parse OK false otherwise
	/// if false is returned, and error message is returned
	bool doParse(std::string& errorMsg);

	/// return the Abstract syntax tree, and release memory
	std::unique_ptr<AstTop> ast() { return std::move(ast_);}

	/// return the Abstract syntax tree, without release memory
	AstTop* getAst() const { return ast_.get();}

private:
	std::unique_ptr<AstTop> ast_;
	std::string           expr_;
};

// This class was added to mitigate the slowness of the boost classic spirit parser
// we will recognise very simple expression, and bypass spirit. Very limited
// But the simple expression do form a very large subset
class SimpleExprParser {
private:
  SimpleExprParser(const SimpleExprParser&) = delete;
  const SimpleExprParser& operator=(const SimpleExprParser&) = delete;
public:
   explicit SimpleExprParser(const std::string& expression) : expr_(expression) {}

   /// Parse the expression, return true if parse OK false otherwise
   bool doParse();

   /// return the Abstract syntax tree, and release memory
   std::unique_ptr<AstTop> ast() { return std::move(ast_);}

private:
   const std::string& expr_;
   std::unique_ptr<AstTop> ast_;
};

#endif
