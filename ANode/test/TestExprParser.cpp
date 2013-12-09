#define BOOST_TEST_MODULE TestNode
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "ExprParser.hpp"
#include "ExprAst.hpp"
#include "Expression.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
using namespace std;


BOOST_AUTO_TEST_SUITE( NodeTestSuite )


BOOST_AUTO_TEST_CASE( test_expression_parser_basic )
{
   std::cout <<  "ANode:: ...test_expression_parser_basic\n";

   // This must be nicely formatted, i.e. AST is nicely space formatted otherwise it will fail the test
   // This test ENSURES the the AST matches the expression. (i.e by getting AST to print the expression)
   // Note: we can use NOT,eq,ne,le,ge, or brackets
   //       we can't use a:event_name  ==> a:event_name == set
   std::vector<std::string> vec;
   vec.push_back("a == complete");
   vec.push_back("a != complete");
   vec.push_back("a:value == 10");
   vec.push_back("a:value != 10");
   vec.push_back("a:value >= 10");
   vec.push_back("a:value <= 10");
   vec.push_back("a:value > 10");
   vec.push_back("a:value < 10");
   vec.push_back("1 == 1");
   vec.push_back("a:event_name == set");
   vec.push_back("a:event_name != set");
   vec.push_back("a:event_name == clear");
   vec.push_back("a:event_name != clear");
   vec.push_back("../a/b:eventname == set");
   vec.push_back("../a/b:eventname == clear");
   vec.push_back("../a/b:eventname != clear");
   vec.push_back("../a:event_name >= 10");
   vec.push_back("a == unknown and b != complete");
   vec.push_back("a == unknown or b != complete");
   vec.push_back("a == complete and b == complete or c == complete");
   vec.push_back("! a == unknown");
   vec.push_back("/mc/main:YMD <= /mc/main/ref:MC_STOP");
   vec.push_back("! ../../../prod2diss/operation_is_late:yes == set or ! a == complete");
   vec.push_back("./a:YMD - ./b:YMD < 5");
   vec.push_back("./a:YMD + ./b:YMD < 5");
   vec.push_back("./a:YMD / ./b:YMD < 5");
   vec.push_back("./a:YMD * ./b:YMD < 5");
   vec.push_back("./a:YMD % ./b:YMD < 5");
   vec.push_back("inigroup:YMD == ! 1");
   vec.push_back("inigroup:YMD == ! 0");

   for(size_t i = 0; i < vec.size(); i++) {

      PartExpression part(vec[i]);
      string parseErrorMsg;
      std::auto_ptr<AstTop> ast = part.parseExpressions( parseErrorMsg );
      BOOST_REQUIRE_MESSAGE(ast.get(),"Failed to parse " << vec[i] << "  " << parseErrorMsg);

      std::stringstream s2;
      ast->print_flat(s2);
      std::string ast_expr = s2.str();
      BOOST_CHECK_MESSAGE(vec[i]==ast_expr," Failed '" << vec[i] << "' != '" << ast_expr << "'" );
   }
}

BOOST_AUTO_TEST_CASE( test_expression_parser_basic_with_brackets )
{
   std::cout <<  "ANode:: ...test_expression_parser_basic_with_brackets\n";

   // This must be nicely formatted, i.e. AST is nicely space formatted otherwise it will fail the test
   // This test ENSURES the the AST matches the expression. (i.e by getting AST to print the expression)
   // Note: we can use NOT,eq,ne,le,ge,
   //       we can't use a:event_name  ==> a:event_name == set
   std::vector<std::string> vec;
   vec.push_back("((a == complete) and (b == complete))");
   vec.push_back("(((a == complete) or (b == complete)) and (c == complete))");
   vec.push_back("((a == complete) and ((b == complete) or (nodepath:eventname == set)))");
   vec.push_back("((a == complete) and ((b == complete) or ((a == complete) and (b == complete))))");
   vec.push_back("! ((a == unknown))");
   vec.push_back("((t:step + 20) >= (t:step1 - 20))");
   vec.push_back("(((/o/main/12/an/slwet == complete) and ((/o/main/12/an/4dvar/ifstraj:finalwave == set) or (/o/main/12/an/4dvar == complete))) or (/o/main/12/an == complete))");
   vec.push_back("((obs:YMD <= (main:YMD + 1)) and ((../make/setup == complete) and (obs:YMD <= /o/lag:YMD)))");
   vec.push_back("(((stage == complete) or (./stage:YMD > ./retrieve:YMD)) and ((./retrieve:YMD - ./load:YMD) < 5))");
   vec.push_back("((./a:YMD - ./b:YMD) < 5)");

   for(size_t i = 0; i < vec.size(); i++) {

      PartExpression part(vec[i]);
      string parseErrorMsg;
      std::auto_ptr<AstTop> ast = part.parseExpressions( parseErrorMsg );
      BOOST_REQUIRE_MESSAGE(ast.get(),"Failed to parse " << vec[i] << "  " << parseErrorMsg);

      std::stringstream s2;
      ast->print_flat(s2,true/*add_brackets*/);
      std::string ast_expr = s2.str();
      BOOST_CHECK_MESSAGE(vec[i]==ast_expr," Failed '" << vec[i] << "' != '" << ast_expr << "'" );
   }
}

BOOST_AUTO_TEST_CASE( test_parser_good_expressions )
{
    std::cout <<  "ANode:: ...test_parser_good_expressions\n";

	// The map key  = trigger expression,
    // value.first  = type of the expected root abstract syntax tree
    // value.second = result of expected evaluation
	map<string,std::pair<string,bool> > exprMap;

   exprMap["a:value == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap["a:value == 10"] = std::make_pair(AstEqual::stype(),false);
   exprMap["a:value eq 10"] = std::make_pair(AstEqual::stype(),false);
   exprMap["a:value != 10"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["a:value ne 10"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["a:value > 10"] = std::make_pair(AstGreaterThan::stype(),false);
   exprMap["a:value gt 10"] = std::make_pair(AstGreaterThan::stype(),false);
   exprMap["a:value >= 10"] = std::make_pair(AstGreaterEqual::stype(),false);
   exprMap["a:value ge 10"] = std::make_pair(AstGreaterEqual::stype(),false);
 	exprMap["a:value < 10"] = std::make_pair(AstLessThan::stype(),true);
   exprMap["a:value lt 10"] = std::make_pair(AstLessThan::stype(),true);
   exprMap["a:value <= 10"] = std::make_pair(AstLessEqual::stype(),true);
   exprMap["a:value le 10"] = std::make_pair(AstLessEqual::stype(),true);

   exprMap["0 == a:value"] = std::make_pair(AstEqual::stype(),true);
   exprMap["10 == a:value"] = std::make_pair(AstEqual::stype(),false);
   exprMap["10 eq a:value"] = std::make_pair(AstEqual::stype(),false);
   exprMap["10 != a:value"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["10 ne a:value"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["10 > a:value"] = std::make_pair(AstGreaterThan::stype(),true);
   exprMap["10 gt a:value"] = std::make_pair(AstGreaterThan::stype(),true);
   exprMap["10 >= a:value"] = std::make_pair(AstGreaterEqual::stype(),true);
   exprMap["10 < a:value"] = std::make_pair(AstLessThan::stype(),false);
   exprMap["10 lt a:value"] = std::make_pair(AstLessThan::stype(),false);
   exprMap["10 <= a:value"] = std::make_pair(AstLessEqual::stype(),false);
   exprMap["10 le a:value"] = std::make_pair(AstLessEqual::stype(),false);

	exprMap["a == complete"] = std::make_pair(AstEqual::stype(),false);
	exprMap["a==complete"]   = std::make_pair(AstEqual::stype(),false);
	exprMap["a eq complete"] = std::make_pair(AstEqual::stype(),false);
	exprMap["a ne complete"] = std::make_pair(AstNotEqual::stype(),true);

   exprMap["0 eq 1"] = std::make_pair(AstEqual::stype(),false);
   exprMap["1000 eq 9"] = std::make_pair(AstEqual::stype(),false);
   exprMap["10 eq 10"] = std::make_pair(AstEqual::stype(),true);
   exprMap["10 ge 4"] = std::make_pair(AstGreaterEqual::stype(),true);
   exprMap["10 le 4"] = std::make_pair(AstLessEqual::stype(),false);

   exprMap["0 == 1"] = std::make_pair(AstEqual::stype(),false);
   exprMap["0 != 1"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["0 < 1"] = std::make_pair(AstLessThan::stype(),true);
   exprMap["10 < 1"] = std::make_pair(AstLessThan::stype(),false);
   exprMap["1000 == 9"] = std::make_pair(AstEqual::stype(),false);
   exprMap["10 == 10"] = std::make_pair(AstEqual::stype(),true);
   exprMap["10 >= 4"] = std::make_pair(AstGreaterEqual::stype(),true);
   exprMap["10 <= 4"] = std::make_pair(AstLessEqual::stype(),false);
   exprMap["0 > 1"] = std::make_pair(AstGreaterThan::stype(),false);
   exprMap["10 > 1"] = std::make_pair(AstGreaterThan::stype(),true);

	exprMap["a:eventname"] = std::make_pair(AstEqual::stype(),false);
	exprMap["./a/b:eventname"] = std::make_pair(AstEqual::stype(),false);
	exprMap["/a/b:eventname"] = std::make_pair(AstEqual::stype(),false);
	exprMap["../a/b:eventname == set"] = std::make_pair(AstEqual::stype(),false);
	exprMap["../a/b:eventname == clear"] = std::make_pair(AstEqual::stype(),true);
	exprMap["../a/b:eventname != clear"] = std::make_pair(AstNotEqual::stype(),false);
   exprMap["a:eventname == set"] = std::make_pair(AstEqual::stype(),false);
   exprMap["a:eventname != set"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["a:eventname == clear"] = std::make_pair(AstEqual::stype(),true);

	exprMap["a:metername >= 100"] = std::make_pair(AstGreaterEqual::stype(),false);
	exprMap["b:metername le 100"] = std::make_pair(AstLessEqual::stype(),true);
	exprMap["./b:metername <= 100"] = std::make_pair(AstLessEqual::stype(),true);
	exprMap["../a/b:metername ge 100"] = std::make_pair(AstGreaterEqual::stype(),false);
	exprMap["../a/b/c:metername >= 100"] = std::make_pair(AstGreaterEqual::stype(),false);

	exprMap["./a == unknown"] = std::make_pair(AstEqual::stype(),true);
	exprMap["./a/b != queued"] = std::make_pair(AstNotEqual::stype(),true);
	exprMap["../a == complete"] = std::make_pair(AstEqual::stype(),false);
	exprMap["../a/b == aborted"] = std::make_pair(AstEqual::stype(),false);
	exprMap["../a/b/c != aborted"] = std::make_pair(AstNotEqual::stype(),true);

	exprMap["a eq unknown and b ne complete"] = std::make_pair(AstAnd::stype(),true);
	exprMap["a eq complete or b eq complete"] = std::make_pair(AstOr::stype(),false);
	exprMap["a eq complete or b eq unknown"] = std::make_pair(AstOr::stype(),true);
	exprMap["a eq complete and b eq complete"] = std::make_pair(AstAnd::stype(),false);
	exprMap["(a eq complete and b == complete)"] = std::make_pair(AstAnd::stype(),false);

   exprMap["a == unknown && b != complete"] = std::make_pair(AstAnd::stype(),true);
   exprMap["a == complete || b == complete"] = std::make_pair(AstOr::stype(),false);
   exprMap["a == complete || b == unknown"] = std::make_pair(AstOr::stype(),true);
   exprMap["a eq complete && b eq complete"] = std::make_pair(AstAnd::stype(),false);
   exprMap["(a == complete && b == complete)"] = std::make_pair(AstAnd::stype(),false);

	// This should be interpreted as '(a == complete and b == complete) or c == complete'
	// because 'and' has a higher priority than the 'or'. Hence 'OR' must be at the root.
   exprMap["a == complete and b == complete or c == complete"] = std::make_pair(AstOr::stype(),false);
   exprMap["a == complete &&  b == complete || c == complete"] = std::make_pair(AstOr::stype(),false);
	exprMap["a == complete and b == complete or c == unknown"] = std::make_pair(AstOr::stype(),true);
	exprMap["a == complete and (b == complete or c == complete)"] = std::make_pair(AstAnd::stype(),false);
	exprMap["a == complete or b == complete and c == complete"] = std::make_pair(AstOr::stype(),false);
	exprMap["((a == complete or b == complete)) and c == complete"] = std::make_pair(AstAnd::stype(),false);

	exprMap["(a != aborted and b == unknown or c != queued)"] = std::make_pair(AstOr::stype(),true);
 	exprMap["(a == complete and b == complete) or nodepath:eventname"] = std::make_pair(AstOr::stype(),false);
	exprMap["(a == complete and b == complete) or (a == complete and b == complete)"] = std::make_pair(AstOr::stype(),false);
	exprMap["a == complete and (b == complete or a == complete) and b == complete"] = std::make_pair(AstAnd::stype(),false);

	// Expression that initially fail to parse for the operational suites
   exprMap["(/skull/consumer/admin/leader:1 and (0 le /skull/consumer/produce1/produce:STEP)) or (not /skull/consumer/admin/leader:1)"] = std::make_pair(AstOr::stype(),true);
	exprMap["./pdb eq complete and  (  not ../../../prod2diss/operation_is_late:yes or ../000/q2diss eq complete)"] = std::make_pair(AstAnd::stype(),false);
   exprMap["! ../../../prod2diss//operation_is_late:yes"] = std::make_pair(AstNot::stype(),true);
 	exprMap["not ../../../prod2diss//operation_is_late:yes"] = std::make_pair(AstNot::stype(),true);
 	exprMap["not ../../../prod2diss/operation_is_late:yes"] = std::make_pair(AstNot::stype(),true);
 	exprMap["not ../../../prod2diss/operation_is_late:yes or a == complete "] = std::make_pair(AstOr::stype(),true);
	exprMap["not ../../../prod2diss/operation_is_late:yes or not a == complete "] = std::make_pair(AstOr::stype(),true);
 	exprMap["not ( a == complete )"] = std::make_pair(AstNot::stype(),true);
 	exprMap["not ( a == unknown )"] = std::make_pair(AstNot::stype(),false);
   exprMap["~ ( a == unknown )"] = std::make_pair(AstNot::stype(),false);
   exprMap["~ ( a != unknown )"] = std::make_pair(AstNot::stype(),true);
   exprMap["! ( a == unknown )"] = std::make_pair(AstNot::stype(),false);
   exprMap["!( a == unknown )"] = std::make_pair(AstNot::stype(),false);
 	exprMap["inigroup:YMD eq ~ 1"] = std::make_pair(AstEqual::stype(),true);
   exprMap["inigroup:YMD eq ~ 0"] = std::make_pair(AstEqual::stype(),false);
   exprMap["inigroup:YMD eq ! 0"] = std::make_pair(AstEqual::stype(),false);
	exprMap["/net/main:YMD le /net/cleanplus1:YMD and 1"] = std::make_pair(AstAnd::stype(),true);

 	exprMap["bins/wamabs eq complete and links eq complete"] = std::make_pair(AstAnd::stype(),false);
 	exprMap["/mc/main:YMD le /mc/main/ref:MC_STOP"] = std::make_pair(AstLessEqual::stype(),true);
 	exprMap["/mc//main:YMD le /mc/main//ref:MC_STOP"] = std::make_pair(AstLessEqual::stype(),true);
 	exprMap["(  ( /o/main/12/an/slwet eq complete and  ( /o/main/12/an/4dvar/ifstraj:finalwave or /o/main/12/an/4dvar eq complete)) or /o/main/12/an eq complete)"] = std::make_pair(AstOr::stype(),false);
	exprMap["../../sv/getsvs eq complete and  ( getae:1 or getae eq complete)"] = std::make_pair(AstAnd::stype(),false);
	exprMap["(  ( /o/lag:YMD gt /sync/o/o/lag:YMD) or  ( /o/main:YMD gt /sync/o/o/main:YMD) or 1 eq 0) and /sync/o ne active and /sync/o ne submitted"] = std::make_pair(AstAnd::stype(),false);


 	exprMap["t:step + 20 le 19"] = std::make_pair(AstLessEqual::stype(),false);
 	exprMap["(t:step + 20) le 19"] = std::make_pair(AstLessEqual::stype(),false);
 	exprMap["t:step + 20 ge 120"] = std::make_pair(AstGreaterEqual::stype(),false);
 	exprMap["t:step - 20 ge 120"] = std::make_pair(AstGreaterEqual::stype(),false);
 	exprMap["t:step + 20 ge t:step1 - 20"] = std::make_pair(AstGreaterEqual::stype(),true);
 	exprMap["(t:step + 20) ge (t:step1 - 20)"] = std::make_pair(AstGreaterEqual::stype(),true);

 	// Note: t:step will evaluate to 0,  0 % number  == 0, however 20 % 0 is a runtime error, same as divide by zero
   exprMap["t:step % 20 < 19"] = std::make_pair(AstLessThan::stype(),true);
   exprMap["t:step % 10 == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap["t:step % 20 ge 19"] = std::make_pair(AstGreaterEqual::stype(),false);
   exprMap["(t:step % 20) ge 19"] = std::make_pair(AstGreaterEqual::stype(),false);
   exprMap["t:step % 20 ge 120"] = std::make_pair(AstGreaterEqual::stype(),false);
   exprMap["t:step % 20 ge 120"] = std::make_pair(AstGreaterEqual::stype(),false);
   exprMap["t:step % 20 ge t:step1 - 20"] = std::make_pair(AstGreaterEqual::stype(),true);
   exprMap["(t:step % 20) ge (t:step1 - 20)"] = std::make_pair(AstGreaterEqual::stype(),true);
   exprMap["(t:step % 20) == (t:step1 % 10)"] = std::make_pair(AstEqual::stype(),true);

 	exprMap["( obs:YMD le  ( main:YMD + 1)) and ../make/setup eq complete and  ( obs:YMD le /o/lag:YMD)"] = std::make_pair(AstAnd::stype(),false);
 	exprMap["( stage eq complete or ./stage:YMD gt ./retrieve:YMD) and ( ./retrieve:YMD - ./load:YMD lt 5)"] = std::make_pair(AstAnd::stype(),false);
 	exprMap["./a:YMD - ./b:YMD lt 5"] = std::make_pair(AstLessThan::stype(),true);


 	std::pair<string, std::pair<string,bool> > p;
	BOOST_FOREACH(p, exprMap ) {

	   ExprParser theExprParser(p.first);
		std::string errorMsg;
		bool ok = theExprParser.doParse(errorMsg);
		BOOST_REQUIRE_MESSAGE(ok,errorMsg);

		string expectedRootType       = p.second.first;
		bool expectedEvaluationResult = p.second.second;

		Ast* top = theExprParser.getAst();
		BOOST_REQUIRE_MESSAGE( top ,"No abstract syntax tree");
 		BOOST_CHECK_MESSAGE( top->left() ,"No root created");
 		BOOST_CHECK_MESSAGE( top->left()->isRoot() ,"First child of top should be a root");
 		BOOST_CHECK_MESSAGE( top->left()->type() == expectedRootType,"expected root type " << expectedRootType << " but found " << top->left()->type());
 		BOOST_CHECK_MESSAGE( expectedEvaluationResult == top->evaluate(),"evaluation not as expected for " << *top);

 		std::string error_msg;
      BOOST_CHECK_MESSAGE(  top->check(error_msg),error_msg << ":  Check failed for " << *top);
	}
}


BOOST_AUTO_TEST_CASE( test_trigger_expression_divide_by_zero )
{
   std::cout <<  "ANode:: ...test_trigger_expression_divide_by_zero\n";

   // The map key  = trigger expression,
   // value.first  = type of the expected root abstract syntax tree
   // value.second = result of expected evaluation
   map<string,std::pair<string,bool> > exprMap;

   exprMap["./a:YMD % 0 == 0"] = std::make_pair(AstEqual::stype(),false);
   exprMap["./a:YMD / 0 == 0"] = std::make_pair(AstEqual::stype(),false);


   std::pair<string, std::pair<string,bool> > p;
   BOOST_FOREACH(p, exprMap ) {

      ExprParser theExprParser(p.first);
      std::string errorMsg;
      bool ok = theExprParser.doParse(errorMsg);
      BOOST_REQUIRE_MESSAGE(ok,errorMsg);

      string expectedRootType       = p.second.first;

      Ast* top = theExprParser.getAst();
      BOOST_REQUIRE_MESSAGE( top ,"No abstract syntax tree");
      BOOST_CHECK_MESSAGE( top->left() ,"No root created");
      BOOST_CHECK_MESSAGE( top->left()->isRoot() ,"First child of top should be a root");
      BOOST_CHECK_MESSAGE( top->left()->type() == expectedRootType,"expected root type " << expectedRootType << " but found " << top->left()->type());

      // expect check to fail, due to divide/modulo by zero
      std::string error_msg;
      BOOST_CHECK_MESSAGE( !top->check(error_msg),error_msg << ":  Check failed for " << *top);
   }
}

BOOST_AUTO_TEST_CASE( test_parser_bad_expressions )
{
    std::cout <<  "ANode:: ...test_parser_bad_expressions\n";
	vector<string> exprvec;
   exprvec.push_back("a = complete");
   exprvec.push_back("a e complete");
   exprvec.push_back("a=complete");
   exprvec.push_back("a ! complete");
   exprvec.push_back("a==complet e");
   exprvec.push_back("a eq complet e");
	exprvec.push_back("a::eventname");
	exprvec.push_back("a:eventname =  set");
	exprvec.push_back("a:eventname == ");
   exprvec.push_back("a:eventname !  set");
   exprvec.push_back("a:eventname ! = set");
   exprvec.push_back("a:eventname %");
 	exprvec.push_back("a:metername  100");
   exprvec.push_back(". == complete");
   exprvec.push_back("/ == complete");
   exprvec.push_back(". == error");
   exprvec.push_back("./ == error");
   exprvec.push_back(".a == error");
   exprvec.push_back(".a == unknown");
   exprvec.push_back(".a/. == unknown");
   exprvec.push_back(".. == unknown");
	exprvec.push_back(".a/b == queued");
	exprvec.push_back("./a/b/ == active");
   exprvec.push_back("..a == complete");
   exprvec.push_back(".../a == complete");
   exprvec.push_back("../.../a == complete");
   exprvec.push_back(".. /a == complete");
   exprvec.push_back("../.. /a == complete");
   exprvec.push_back("../../.a == complete");
	exprvec.push_back("..a/b == aborted");
	exprvec.push_back("..a/b/c == aborted");
   exprvec.push_back("a == complete and");
   exprvec.push_back("a %");
	exprvec.push_back("(a == complete   b == complete)");
	exprvec.push_back("a == complete and  b == complete)");
	exprvec.push_back("(a == complete and  b == complete");
	exprvec.push_back("(a = complete and b = complete or c = complete)");
	exprvec.push_back("(a erro complete and b == complete) or nodepath:eventname");
	exprvec.push_back("(a == complete and b == complete or (a == complete and b == complete)");
    // triggers that dont make sense in the operational suites.
	exprvec.push_back("../../../legA/fc/pf/01 eq complete eq complete");

	BOOST_FOREACH(const string& expr, exprvec ) {

		//std::cout << "parsing expression " << expr << "\n";
		ExprParser theExprParser(expr);
		std::string errorMsg;
 		BOOST_CHECK_MESSAGE( !theExprParser.doParse( errorMsg), expr << " expected to fail " );
 	}
}

BOOST_AUTO_TEST_SUITE_END()

