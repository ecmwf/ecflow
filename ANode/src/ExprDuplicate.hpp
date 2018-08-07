#ifndef EXPR_DUPLICATE_
#define EXPR_DUPLICATE_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
// =========================================================================
// For large designs > 90% of triggers are identical.
// We take advantage of this by using a map to store a *CLONED* ast
// This saves a huge amount on re-parsing using spirit classic.
//
// This cloned  AST is maintained in a static map, hence we need to
// manage the lifetime, to avoid valgrind complaining.
// =========================================================================

#include <string>
#include <memory> // for unique_ptr
#include <boost/noncopyable.hpp>
class AstTop;

// reclaim memory allocated in map, Avoid valgrind errors
class ExprDuplicate : private boost::noncopyable {
public:
   ExprDuplicate() = default;
   ~ExprDuplicate();

   // for debug only
   static void dump(const std::string& msg );

   // Find the expr in the map, if found returns a CLONED ast, else NULL
   static std::unique_ptr<AstTop> find(const std::string& expr);

   // Add the expr to the map, the ast is cloned.
   static void add(const std::string& expr,AstTop*);
};

#endif
