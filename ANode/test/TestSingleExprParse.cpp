#define BOOST_TEST_MODULE TestSingle
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "ExprParser.hpp"
#include "ExprAst.hpp"
#include "ExprDuplicate.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib, for to_simple_string

#include <string>
#include <map>
#include <iostream>
#include <fstream>
using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

// DEBUG AID: to see the expression tree, invert the expected evaluation
//            so that test fail's

BOOST_AUTO_TEST_SUITE( NodeTestSuite )


BOOST_AUTO_TEST_CASE( test_single_expression )
{
    std::cout <<  "ANode:: ...test_single_expression\n";

    // Duplicate AST are held in a static map. Delete them, to avoid ASAN complaining
    ExprDuplicate reclaim_cloned_ast_memory;

    // The map key = trigger expression,
    // value.first  = type of expected root abstract syntax tree
    // value.second = result of expected evaluation
    map<string,std::pair<string,bool> > exprMap;

   exprMap[":var == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap[":var != 1"] = std::make_pair(AstNotEqual::stype(),true);

 	std::pair<string, std::pair<string,bool> > p;
	BOOST_FOREACH(p, exprMap ) {

  		ExprParser theExprParser(p.first);
		std::string errorMsg;
		bool ok = theExprParser.doParse(errorMsg);
		BOOST_REQUIRE_MESSAGE(ok,errorMsg);

		string expectedRootType       = p.second.first;
		bool expectedEvaluationResult = p.second.second;

      std::stringstream ss;

		Ast* top = theExprParser.getAst();
		BOOST_REQUIRE_MESSAGE( top ,"No abstract syntax tree");
		BOOST_REQUIRE_MESSAGE( top->left() ,"No root created");
		BOOST_REQUIRE_MESSAGE( top->left()->isRoot() || top->left()->is_attribute() ,"First child of top should be a root or attribute " + p.first);
		BOOST_REQUIRE_MESSAGE( top->left()->is_evaluateable(),"expected ast to be evaluatable. found: " << top->left()->type() << " " << p.first);
		BOOST_REQUIRE_MESSAGE( top->left()->type() == expectedRootType || top->left()->type() == "variable","expected root type " << expectedRootType << " or 'variable' but found " << top->left()->type() << " " << p.first);
      top->print_flat(ss);
		BOOST_REQUIRE_MESSAGE( expectedEvaluationResult == top->evaluate(),"evaluation not as expected for:\n" << p.first << "\n" << ss.str() << "\n" << *top);

		std::string why;
		top->why(why);
		cout << "why: " << why << "\n";
	}
}


//BOOST_AUTO_TEST_CASE( test_expression_read_from_file )
//{
//   std::cout <<  "ANode:: ...test_expression_read_from_file\n";
//
//   std::string filename = "/var/tmp/ma0/BIG_DEFS/triggers.list";
//   std::ifstream the_file(filename.c_str(),std::ios_base::in);
//   BOOST_REQUIRE_MESSAGE(the_file,"file " << filename << "not found");
//
//   int  i = 0;
//   string line;
//   while ( std::getline(the_file,line) ) {
//      i++;
//      // cout << i << ": " << line << "\n";
//      ExprParser theExprParser(line);
//      std::string errorMsg;
//      bool ok = theExprParser.doParse(errorMsg);
//      BOOST_CHECK_MESSAGE(ok,errorMsg << " line: " << i );
//
//      if (ok) {
//         Ast* top = theExprParser.getAst();
//         BOOST_REQUIRE_MESSAGE( top ,"No abstract syntax tree " + line);
//         BOOST_REQUIRE_MESSAGE( top->left() ,"No root created " + line);
//         BOOST_REQUIRE_MESSAGE( top->left()->isRoot() || top->left()->is_attribute() ,"First child of top should be a root or variable " + line);
//         //top->print_flat(ss);
//      }
//   }
//
//   ExprDuplicate reclaim_cloned_ast_memory;
//   cout << " read " << i << " expressions\n";
//}

BOOST_AUTO_TEST_SUITE_END()
