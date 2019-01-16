#define BOOST_TEST_MODULE TestNode
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>
#include <map>
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib, for to_simple_string

#include "ExprParser.hpp"
#include "ExprAst.hpp"
#include "Expression.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )


BOOST_AUTO_TEST_CASE( test_expression_parser_basic )
{
   std::cout <<  "ANode:: ...test_expression_parser_basic\n";

   // This must be nicely formatted, i.e. AST is nicely space formatted otherwise it will fail the test
   // This test ENSURES the the AST matches the expression. (i.e by getting AST to print the expression)
   // Note: we can use NOT,eq,ne,le,ge, or brackets
   //       we can't use a:event_name  ==> a:event_name == set
   std::vector<std::string> vec;
   vec.emplace_back("a == complete");
   vec.emplace_back("a != complete");
   vec.emplace_back("a:value == 10");
   vec.emplace_back("a:value != 10");
   vec.emplace_back("a:value >= 10");
   vec.emplace_back("a:value <= 10");
   vec.emplace_back("a:value > 10");
   vec.emplace_back("a:value < 10");
   vec.emplace_back("1 == 1");
   vec.emplace_back("1 == 0");
   vec.emplace_back("a:event_name == set");
   vec.emplace_back("a:event_name != set");
   vec.emplace_back("a:event_name == clear");
   vec.emplace_back("a:event_name != clear");
   vec.emplace_back("../a/b:eventname == set");
   vec.emplace_back("../a/b:eventname == clear");
   vec.emplace_back("../a/b:eventname != clear");
   vec.emplace_back("../a:event_name >= 10");
   vec.emplace_back("a == unknown and b != complete");
   vec.emplace_back("a == unknown or b != complete");
   vec.emplace_back("a == complete and b == complete or c == complete");
   vec.emplace_back("! a == unknown");
   vec.emplace_back("/mc/main:YMD <= /mc/main/ref:MC_STOP");
   vec.emplace_back("! ../../../prod2diss/operation_is_late:yes == set or ! a == complete");
   vec.emplace_back("./a:YMD - ./b:YMD < 5");
   vec.emplace_back("./a:YMD + ./b:YMD < 5");
   vec.emplace_back("./a:YMD / ./b:YMD < 5");
   vec.emplace_back("./a:YMD * ./b:YMD < 5");
   vec.emplace_back("./a:YMD % ./b:YMD < 5");
   vec.emplace_back("inigroup:YMD == ! 1");
   vec.emplace_back("inigroup:YMD == ! 0");
   vec.emplace_back("comp == complete and notready == complete");  // ECFLOW-493
   vec.emplace_back("comp == complete and not ready == complete");
   vec.emplace_back("comp == complete and ! ready == complete");   // we now store the not from the parse, for test comparison
   vec.emplace_back("comp == complete and ~ ready == complete");

   vec.emplace_back(":VAR == 1");
   vec.emplace_back(":VAR == /mc/main/ref:MC_STOP");
   vec.emplace_back(":YMD - :YMD < 5");
   vec.emplace_back(":YMD + :YMD < 5");
   vec.emplace_back(":YMD / :YMD < 5");
   vec.emplace_back(":YMD * :YMD < 5");
   vec.emplace_back(":YMD % :YMD < 5");

   for(const auto & i : vec) {

      PartExpression part(i);
      string parseErrorMsg;
      std::unique_ptr<AstTop> ast = part.parseExpressions( parseErrorMsg );
      BOOST_REQUIRE_MESSAGE(ast.get(),"Failed to parse\n" << i << "  " << parseErrorMsg);

      std::stringstream s2;
      ast->print_flat(s2);
      std::string ast_expr = s2.str();
      BOOST_CHECK_MESSAGE(i==ast_expr," Failed\n'" << i << "' != '" << ast_expr << "'" );

      std::string why; ast->why(why);
      //cout << "why: " << vec[i] << " -> " << why << "\n";
      if (ast->evaluate()) {
         BOOST_CHECK_MESSAGE(why.empty(),"Expected why to be empty when expression evaluates: " << i);
      }
      else {
         BOOST_CHECK_MESSAGE(!why.empty(),"When ast does not evaluate we expect to find why: " << i);
      }
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
   vec.emplace_back("((a == complete) and (b == complete))");
   vec.emplace_back("(((a == complete) or (b == complete)) and (c == complete))");
   vec.emplace_back("((a == complete) and ((b == complete) or (nodepath:eventname == set)))");
   vec.emplace_back("((a == complete) and ((b == complete) or ((a == complete) and (b == complete))))");
   vec.emplace_back("! ((a == unknown))");
   vec.emplace_back("((t:step + 20) >= (t:step1 - 20))");
   vec.emplace_back("(((/o/main/12/an/slwet == complete) and ((/o/main/12/an/4dvar/ifstraj:finalwave == set) or (/o/main/12/an/4dvar == complete))) or (/o/main/12/an == complete))");
   vec.emplace_back("((obs:YMD <= (main:YMD + 1)) and ((../make/setup == complete) and (obs:YMD <= /o/lag:YMD)))");
   vec.emplace_back("(((stage == complete) or (./stage:YMD > ./retrieve:YMD)) and ((./retrieve:YMD - ./load:YMD) < 5))");
   vec.emplace_back("((./a:YMD - ./b:YMD) < 5)");
   vec.emplace_back("((:YMD + :YMD) < 5)");

   for(const auto & i : vec) {

      PartExpression part(i);
      string parseErrorMsg;
      std::unique_ptr<AstTop> ast = part.parseExpressions( parseErrorMsg );
      BOOST_REQUIRE_MESSAGE(ast.get(),"Failed to parse " << i << "  " << parseErrorMsg);

      std::stringstream s2;
      ast->print_flat(s2,true/*add_brackets*/);
      std::string ast_expr = s2.str();
      BOOST_CHECK_MESSAGE(i==ast_expr," Failed '" << i << "' != '" << ast_expr << "'" );

      std::string why; ast->why(why);
      //cout << "why: " << vec[i] << " -> " << why << "\n";
      if (ast->evaluate()) {
         BOOST_CHECK_MESSAGE(why.empty(),"Expected why to be empty when expression evaluates: " << i);
      }
      else {
         BOOST_CHECK_MESSAGE(!why.empty(),"When ast does not evaluate we expect to find why: " << i);
      }
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

   exprMap["0 == 0"] = std::make_pair(AstEqual::stype(),true);
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
   exprMap["a:eventname == 0"] = std::make_pair(AstEqual::stype(),true);
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

   exprMap["!a:a  &&   b:b"] = std::make_pair(AstAnd::stype(),false);
   exprMap["a:a   && ! b:b"] = std::make_pair(AstAnd::stype(),false);
   exprMap["! a:a ||   b:b"] = std::make_pair(AstOr::stype(),true);
   exprMap["a:a   || ! b:b"] = std::make_pair(AstOr::stype(),true);
   exprMap["! a:a &&   b:b &&   c:c"] = std::make_pair(AstAnd::stype(),false);
   exprMap["! a:a &&   b:b ||   c:c"] = std::make_pair(AstOr::stype(),false);
   exprMap["a:a && !   b:b &&   c:c"] = std::make_pair(AstAnd::stype(),false);
   exprMap["a:a && !   b:b ||   c:c"] = std::make_pair(AstOr::stype(),false);
   exprMap["a:a ||     b:b && ! c:c"] = std::make_pair(AstOr::stype(),false);
   exprMap["a:a ||     b:b || ! c:c"] = std::make_pair(AstOr::stype(),true);

   exprMap["a:b && b:c && c:d && ../c:b && ./x:y && z:x"] = std::make_pair(AstAnd::stype(),false);
   exprMap["! a:b && b:c && c:d && ../c:b && ./x:y && z:x"] = std::make_pair(AstAnd::stype(),false);
   exprMap["! a:b || b:c && c:d && ../c:b && ./x:y && z:x"] = std::make_pair(AstOr::stype(),true);
   exprMap["a:b || b:c && ! c:d && ../c:b && ./x:y && z:x"] = std::make_pair(AstOr::stype(),false);
   exprMap["a:b && b:c && c:d && ../c:b && ./x:y && ! z:x"] = std::make_pair(AstAnd::stype(),false);
   exprMap["x:x || a:b && b:c && c:d && ../c:b && ./x:y && z:x"] = std::make_pair(AstOr::stype(),false);
   exprMap["x:x == 0 || a:b && b:c && c:d && ../c:b && ./x:y && z:x"] = std::make_pair(AstOr::stype(),true);
   exprMap["! a:a && ! b:b && ! c:c && ! d:d "] = std::make_pair(AstAnd::stype(),true);
   exprMap["! a:a || ! b:b || ! c:c || ! d:d "] = std::make_pair(AstOr::stype(),true);

   exprMap["./a == unknown"] = std::make_pair(AstEqual::stype(),true);
   exprMap["./a/b != queued"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["../a == complete"] = std::make_pair(AstEqual::stype(),false);
   exprMap["../a/b == aborted"] = std::make_pair(AstEqual::stype(),false);
   exprMap["../a/b/c != aborted"] = std::make_pair(AstNotEqual::stype(),true);

   exprMap["inigroup:YMD == ! 1"] = std::make_pair(AstEqual::stype(),true);
   exprMap["1 != ! 1"] = std::make_pair(AstNotEqual::stype(),true);
   exprMap["b == complete or nodepath:eventname"] = std::make_pair(AstOr::stype(),false);

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
   exprMap["a == complete && b == complete and c == complete or x:a == 0"] = std::make_pair(AstOr::stype(),true);
   exprMap["a == complete || b == complete and c == complete and x:a == 0 || x == complete"] = std::make_pair(AstOr::stype(),false);
   exprMap["a == complete || b == complete and c == complete and x:a || x == complete"] = std::make_pair(AstOr::stype(),false);

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

   exprMap["! ../../../operation_is_late:yes == set"] = std::make_pair(AstNot::stype(),true);
   exprMap["2 == (((/seasplots/lag:YMD / 100 ) % 100) % 3)"] = std::make_pair(AstEqual::stype(),false);
   exprMap["(((/seasplots/lag:YMD / 100 ) % 100) % 3) ==  2"] = std::make_pair(AstEqual::stype(),false);
   exprMap["! ../../../a:yes == set or ! a == complete or y == aborted"] = std::make_pair(AstOr::stype(),true);

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

   exprMap["comp == complete and notready == complete"] = std::make_pair(AstAnd::stype(),false);

   exprMap["/s/f/t<flag>late"] = std::make_pair(AstFlag::stype(),false);
   exprMap["./s<flag>late"] = std::make_pair(AstFlag::stype(),false);
   exprMap["../s/f/t<flag>late"] = std::make_pair(AstFlag::stype(),false);
   exprMap["/s/f/t<flag>late == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap["0 == /s/f/t<flag>late"] = std::make_pair(AstEqual::stype(),true);
   exprMap["/s/f/t<flag>late and /s/f/t<flag>late"] = std::make_pair(AstAnd::stype(),false);
   exprMap["! /s/f/t<flag>late and ! /s/f/t<flag>late"] = std::make_pair(AstAnd::stype(),true);
   exprMap["! /s/f/t<flag>late"] = std::make_pair(AstNot::stype(),true);
   exprMap["/s/f/t<flag>late + 2 >= 2"] = std::make_pair(AstGreaterEqual::stype(),true);
   exprMap["(/s/f/t<flag>late or 1)"] = std::make_pair(AstOr::stype(),true);
   exprMap["/<flag>late"] = std::make_pair(AstFlag::stype(),false);

   exprMap[":VAR == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap[":VAR == 1"] = std::make_pair(AstEqual::stype(),false);
   exprMap[":VAR == /mc/main/ref:MC_STOP"] = std::make_pair(AstEqual::stype(),true);
   exprMap[":YMD - :YMD <= 5"] = std::make_pair(AstLessEqual::stype(),true);
   exprMap[":YMD + :YMD <= 5"] = std::make_pair(AstLessEqual::stype(),true);
   exprMap[":YMD * :YMD <= 5"] = std::make_pair(AstLessEqual::stype(),true);
   exprMap[":YMD + 1 == 1"] = std::make_pair(AstEqual::stype(),true);

   int parse_failure = 0;
   int ast_failure = 0;
   std::pair<string, std::pair<string,bool> > p;
   BOOST_FOREACH(p, exprMap ) {

      //cout << "parsing: " << p.first << "\n";
      ExprParser theExprParser(p.first);
      std::string errorMsg;
      bool ok = theExprParser.doParse(errorMsg);
      if (!ok) parse_failure++;
      BOOST_CHECK_MESSAGE(ok,errorMsg + "failed for " + p.first);

      if (ok) {
         string expectedRootType       = p.second.first;
         bool expectedEvaluationResult = p.second.second;

         Ast* top = theExprParser.getAst();
         if (!top) ast_failure++;
         BOOST_CHECK_MESSAGE( top ,"No abstract syntax tree "+ p.first);
         if (top) {
            BOOST_CHECK_MESSAGE( top->left() ,"No root created "+ p.first);
            BOOST_CHECK_MESSAGE( top->left()->isRoot() || top->left()->is_attribute() ,"First child of top should be a root or attribute " + p.first);
            BOOST_CHECK_MESSAGE( top->left()->is_evaluateable(),"expected ast to be evaluatable. found: " << top->left()->type() << " " << p.first);
            BOOST_CHECK_MESSAGE( top->left()->type() == expectedRootType || top->left()->type() == "variable","expected root type '" << expectedRootType << "' or 'variable' but found '" << top->left()->type() << "' " << p.first);
            BOOST_CHECK_MESSAGE( expectedEvaluationResult == top->evaluate(),"evaluation not as expected for:\n" << p.first << "\n" << *top);

            std::string error_msg;
            BOOST_CHECK_MESSAGE(  top->check(error_msg),error_msg << ":  Check failed for " << *top);


            std::string why; top->why(why);
            //cout << "why: " << p.first << " -> " << why << "\n";
            if (top->evaluate()) {
               BOOST_CHECK_MESSAGE(why.empty(),"Expected why to be empty when expression evaluates: " << p.first);
            }
            else {
               BOOST_CHECK_MESSAGE(!why.empty(),"When ast does not evaluate we expect to find why: " << p.first);
            }
         }
      }
   }
   BOOST_REQUIRE_MESSAGE( parse_failure == 0 &&  ast_failure == 0,"Found failures parse_failure:" << parse_failure << " ast failure:" << ast_failure);
}


BOOST_AUTO_TEST_CASE( test_trigger_functions )
{
    std::cout <<  "ANode:: ...test_trigger_functions\n";

   // The map key  = trigger expression,
    // value.first  = type of the expected root abstract syntax tree
    // value.second = result of expected evaluation
   map<string,std::pair<string,bool> > exprMap;

   exprMap["cal::date_to_julian(A:x) == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap["cal::date_to_julian( 0 ) == cal::date_to_julian( 0 )"] = std::make_pair(AstEqual::stype(),true);
   exprMap["cal::date_to_julian( 0 ) == cal::date_to_julian( A:x )"] = std::make_pair(AstEqual::stype(),true);
   exprMap["2457620 == cal::date_to_julian( 20160819 )"] = std::make_pair(AstEqual::stype(),true);
   exprMap["20160819 == cal::julian_to_date( 2457620 )"] = std::make_pair(AstEqual::stype(),true);

   // test 10 digit integer, ie yyyymmddhh
   exprMap["2457620 == cal::date_to_julian( 2016081912 )"] = std::make_pair(AstEqual::stype(),true);

   int parse_failure = 0;
   int ast_failure = 0;
   std::pair<string, std::pair<string,bool> > p;
   BOOST_FOREACH(p, exprMap ) {

      //cout << "parsing: " << p.first << "\n";
      ExprParser theExprParser(p.first);
      std::string errorMsg;
      bool ok = theExprParser.doParse(errorMsg);
      if (!ok) parse_failure++;
      BOOST_CHECK_MESSAGE(ok,errorMsg + "failed for " + p.first);

      if (ok) {
         string expectedRootType       = p.second.first;
         bool expectedEvaluationResult = p.second.second;

         Ast* top = theExprParser.getAst();
         if (!top) ast_failure++;
         BOOST_CHECK_MESSAGE( top ,"No abstract syntax tree "+ p.first);
         if (top) {
            BOOST_CHECK_MESSAGE( top->left() ,"No root created "+ p.first);
            BOOST_CHECK_MESSAGE( top->left()->isRoot() || top->left()->is_attribute() ,"First child of top should be a root or attribute " + p.first);
            BOOST_CHECK_MESSAGE( top->left()->is_evaluateable(),"expected ast to be evaluatable. found: " << top->left()->type() << " " << p.first);
            BOOST_CHECK_MESSAGE( top->left()->type() == expectedRootType || top->left()->type() == "variable","expected root type '" << expectedRootType << "' or 'variable' but found '" << top->left()->type() << "' " << p.first);
            BOOST_CHECK_MESSAGE( expectedEvaluationResult == top->evaluate(),"evaluation not as expected for:\n" << p.first << "\n" << *top);

            std::string error_msg;
            BOOST_CHECK_MESSAGE(  top->check(error_msg),error_msg << ":  Check failed for " << *top);


            std::string why; top->why(why);
            //cout << "why: " << p.first << " -> " << why << "\n";
            if (top->evaluate()) {
               BOOST_CHECK_MESSAGE(why.empty(),"Expected why to be empty when expression evaluates: " << p.first);
            }
            else {
               BOOST_CHECK_MESSAGE(!why.empty(),"When ast does not evaluate we expect to find why: " << p.first);
            }
         }
      }
   }
   BOOST_REQUIRE_MESSAGE( parse_failure == 0 &&  ast_failure == 0,"Found failures parse_failure:" << parse_failure << " ast failure:" << ast_failure);
}

BOOST_AUTO_TEST_CASE( test_date_to_julian_with_repeat_YMD )
{
    std::cout <<  "ANode:: ...test_date_to_julian_with_repeat_YMD\n";

    Defs theDefs;
    suite_ptr suite = theDefs.add_suite("s1");
    suite->addRepeat( RepeatDate("YMD",20170101,20180101,1));
    task_ptr t1 = suite->add_task("t1");
    t1->add_trigger("2457755 == cal::date_to_julian( /s1:YMD )");
    theDefs.beginAll();

    std::string err_msg,warn_msg;
    theDefs.check(err_msg,warn_msg);
    BOOST_REQUIRE_MESSAGE(err_msg.empty() && warn_msg.empty(),"Expected no errors but found " <<  err_msg << "\nexpected no warnings but found: " << warn_msg);

    // make sure we can resolve /s1:YMD if this the case bottom_up_why should return vector of size 0
    std::vector<std::string> theReasonWhy;
    t1->bottom_up_why(theReasonWhy);
    BOOST_CHECK_MESSAGE(theReasonWhy.empty() ,"When all is well expected empty reason vec");

    // be more flexible, of vector is returned we should not get: variable-not-found
    for(auto & i : theReasonWhy) {
       cout << i << "\n";
       BOOST_CHECK_MESSAGE(i.find("variable-not-found") == string::npos,"Variable YMD not found: " << i);
    }
}

BOOST_AUTO_TEST_CASE( test_trigger_functions_with_boost_date )
{
   std::cout <<  "ANode:: ...test_trigger_functions_with_boost_date\n";
   // The map key  = trigger expression,
   // value.first  = type of the expected root abstract syntax tree
   // value.second = result of expected evaluation
   map<string,std::pair<string,bool> > exprMap;

   boost::gregorian::date startDate(2017,1,1);
   boost::gregorian::date endDate(2017,12,31);
   while(startDate != endDate) {
      long julian_day = startDate.julian_day();
      std::string str_julian_day = boost::lexical_cast<std::string>(julian_day);
      std::string eight_digit_iso_string =  to_iso_string(startDate);
      string expr = str_julian_day + " ==  cal::date_to_julian(" + eight_digit_iso_string + ")";
      exprMap[expr] = std::make_pair(AstEqual::stype(),true);

      std::string ten_digit_string = eight_digit_iso_string + "12" ;
      expr = str_julian_day + " ==  cal::date_to_julian(" + ten_digit_string + ")";
      exprMap[expr] = std::make_pair(AstEqual::stype(),true);

      string expr2 = to_iso_string(startDate) + " == cal::julian_to_date(" + str_julian_day + ")";
      exprMap[expr2] = std::make_pair(AstEqual::stype(),true);
      startDate += days(1);
   }

   int parse_failure = 0;
   int ast_failure = 0;
   std::pair<string, std::pair<string,bool> > p;
   BOOST_FOREACH(p, exprMap ) {

      //cout << "parsing: " << p.first << "\n";
      ExprParser theExprParser(p.first);
      std::string errorMsg;
      bool ok = theExprParser.doParse(errorMsg);
      if (!ok) parse_failure++;
      BOOST_CHECK_MESSAGE(ok,errorMsg + "failed for " + p.first);

      if (ok) {
         string expectedRootType       = p.second.first;
         bool expectedEvaluationResult = p.second.second;

         Ast* top = theExprParser.getAst();
         if (!top) ast_failure++;
         BOOST_CHECK_MESSAGE( top ,"No abstract syntax tree "+ p.first);
         if (top) {
            BOOST_CHECK_MESSAGE( top->left() ,"No root created "+ p.first);
            BOOST_CHECK_MESSAGE( top->left()->isRoot() || top->left()->is_attribute() ,"First child of top should be a root or attribute " + p.first);
            BOOST_CHECK_MESSAGE( top->left()->is_evaluateable(),"expected ast to be evaluatable. found: " << top->left()->type() << " " << p.first);
            BOOST_CHECK_MESSAGE( top->left()->type() == expectedRootType || top->left()->type() == "variable","expected root type '" << expectedRootType << "' or 'variable' but found '" << top->left()->type() << "' " << p.first);
            BOOST_CHECK_MESSAGE( expectedEvaluationResult == top->evaluate(),"evaluation not as expected for:\n" << p.first << "\n" << *top);

            std::string error_msg;
            BOOST_CHECK_MESSAGE(  top->check(error_msg),error_msg << ":  Check failed for " << *top);
         }
      }
   }
   BOOST_REQUIRE_MESSAGE( parse_failure == 0 &&  ast_failure == 0,"Found failures parse_failure:" << parse_failure << " ast failure:" << ast_failure);
}


BOOST_AUTO_TEST_CASE( test_trigger_expression_divide_by_zero )
{
   std::cout <<  "ANode:: ...test_trigger_expression_divide_by_zero\n";

   // The map key  = trigger expression,
   // value.first  = type of the expected root abstract syntax tree
   // value.second = result of expected evaluation
   map<string,std::pair<string,bool> > exprMap;

   // Divide by zero or modulo by zero would lead to run-time crash
   // However the Ast::evaluate() checks for this, and return zero for the whole expression i.e ./a:YMD % 0 returns 0
   exprMap["./a:YMD % 0 == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap["./a:YMD / 0 == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap[":YMD % 0 == 0"] = std::make_pair(AstEqual::stype(),true);
   exprMap[":YMD / 0 == 0"] = std::make_pair(AstEqual::stype(),true);

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

      // expect check to fail, due to divide/modulo by zero
      std::string error_msg;
      BOOST_CHECK_MESSAGE( !top->check(error_msg),error_msg << ":  Check failed for " << *top);
   }
}

BOOST_AUTO_TEST_CASE( test_parser_bad_expressions ) 
{
   std::cout << "ANode:: ...test_parser_bad_expressions\n";
   vector<string> exprvec;
   exprvec.emplace_back("a <= complete");
   exprvec.emplace_back("a >= complete");
   exprvec.emplace_back("a = complete");
   exprvec.emplace_back("a e complete");
   exprvec.emplace_back("a=complete");
   exprvec.emplace_back("a ! complete");
   exprvec.emplace_back("a==complet e");
   exprvec.emplace_back("a eq complet e");
   exprvec.emplace_back("a::eventname");
   exprvec.emplace_back("a:eventname =  set");
   exprvec.emplace_back("a:eventname == ");
   exprvec.emplace_back("a:eventname !  set");
   exprvec.emplace_back("a:eventname ! = set");
   exprvec.emplace_back("a:eventname %");
   exprvec.emplace_back("a:event <= set");
   exprvec.emplace_back("a:event >= set");
   exprvec.emplace_back("a:event >= clear");
   exprvec.emplace_back("a:event >= fred");
   exprvec.emplace_back("a:metername  100");
   exprvec.emplace_back(". == complete");
   exprvec.emplace_back("/ == complete");
   exprvec.emplace_back(". == error");
   exprvec.emplace_back("./ == error");
   exprvec.emplace_back(".a == error");
   exprvec.emplace_back(".a == unknown");
   exprvec.emplace_back(".a/. == unknown");
   exprvec.emplace_back(".. == unknown");
   exprvec.emplace_back(".a/b == queued");
   exprvec.emplace_back("./a/b/ == active");
   exprvec.emplace_back("..a == complete");
   exprvec.emplace_back(".../a == complete");
   exprvec.emplace_back("../.../a == complete");
   exprvec.emplace_back(".. /a == complete");
   exprvec.emplace_back("../.. /a == complete");
   exprvec.emplace_back("../../.a == complete");
   exprvec.emplace_back("..a/b == aborted");
   exprvec.emplace_back("..a/b/c == aborted");
   exprvec.emplace_back("a == complete and");
   exprvec.emplace_back("a %");
   exprvec.emplace_back("(a == complete   b == complete)");
   exprvec.emplace_back("a == complete and  b == complete)");
   exprvec.emplace_back("(a == complete and  b == complete");
   exprvec.emplace_back("(a = complete and b = complete or c = complete)");
   exprvec.emplace_back("(a erro complete and b == complete) or nodepath:eventname");
   exprvec.emplace_back("(a == complete and b == complete or (a == complete and b == complete)");
   // triggers that dont make sense in the operational suites.
   exprvec.emplace_back("../../../legA/fc/pf/01 eq complete eq complete");

   exprvec.emplace_back("/mofc/mon/hind/14/back == complete or %s:DOW ne 5"); // ECFLOW-888


   BOOST_FOREACH(const string& expr, exprvec ) {

      //std::cout << "parsing expression " << expr << "\n";
      ExprParser theExprParser(expr);
      std::string errorMsg;
      BOOST_CHECK_MESSAGE(!theExprParser.doParse(errorMsg), expr << " expected to fail ");
   }
}

BOOST_AUTO_TEST_SUITE_END()
