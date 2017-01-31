//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $
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
#if defined(HPUX)
#include <map>
#else
#include <boost/unordered_map.hpp>
#endif
#include <boost/foreach.hpp>

#include "ExprDuplicate.hpp"
#include "ExprAst.hpp"

////////////////////////////////////////////////////////////////////////////
using namespace std;

#if defined(HPUX)
// boost 1.51 HPUX has problems with boost::unordered_map
static std::map< std::string, AstTop* > duplicate_expr;
typedef std::map< std::string, AstTop* > my_map;
#else
static boost::unordered_map< std::string, AstTop* > duplicate_expr;
typedef boost::unordered_map< std::string, AstTop* > my_map;
#endif


ExprDuplicate::~ExprDuplicate()
{
//   cout << "ExprDuplicate::~ExprDuplicate()\n";
   BOOST_FOREACH(my_map::value_type i, duplicate_expr) {
      AstTop* top = i.second;
      delete top;
   }
   duplicate_expr.clear();
}

std::auto_ptr<AstTop> ExprDuplicate::find(const std::string& expr)
{
   my_map::const_iterator it = duplicate_expr.find(expr);
   if (it != duplicate_expr.end()) {
      return std::auto_ptr<AstTop>((*it).second->clone());
   }
   return std::auto_ptr<AstTop>();
}

void ExprDuplicate::add(const std::string& expr,AstTop* ast)
{
   assert(!expr.empty() && ast);
   duplicate_expr.insert( std::make_pair(expr,ast->clone()));
}
