//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $
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
#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "Str.hpp"
#include "StringSplitter.hpp"

using namespace std;
using namespace ecf;
using namespace boost;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )


static void check(const std::string& line,
                  const StringSplitter& string_splitter,
                  const std::vector<std::string>& expected )
{
   std::vector<std::string> result;
   while (!string_splitter.finished()) {
      boost::string_ref ref = string_splitter.next();
      //std::cout << "ref:'" << ref << "'\n";
      result.push_back(std::string(ref.begin(),ref.end()));
   }
   BOOST_CHECK_MESSAGE(result.size() == expected.size(),"expected size " << expected.size() << " but found " << result.size() << " for '" << line << "'");
   BOOST_CHECK_MESSAGE(result == expected,"failed for '" << line  << "'");
   if (result != expected) {
      cout << "Actual  :"; BOOST_FOREACH(const string& t, result)   { cout << "'" << t << "'"; } cout << "\n";
      cout << "Expected:"; BOOST_FOREACH(const string& t, expected) { cout << "'" << t << "'"; } cout << "\n";
   }
}

BOOST_AUTO_TEST_CASE( test_StringSplitter )
{
   cout << "ACore:: ...test_StringSplitter\n";

   std::string line = "This is a string please split me";
   std::vector<std::string> expected;

   expected.push_back("This"); expected.push_back("is"); expected.push_back("a"); expected.push_back("string");
   expected.push_back("please"); expected.push_back("split"); expected.push_back("me");
   StringSplitter string_splitter(line);
   check(line, string_splitter,expected);

   // reset
   string_splitter.reset();
   check(line, string_splitter,expected);
}


BOOST_AUTO_TEST_CASE( test_str_split_StringSplitter )
{
   cout << "ACore:: ...test_str_split_StringSplitter\n";

   // If end is delimeter, then preserved as empty token

   std::string line = "This is a string";
   std::vector<std::string> expected;

   expected.push_back("This"); expected.push_back("is"); expected.push_back("a"); expected.push_back("string");
   check(line, StringSplitter(line),expected);

   expected.clear();
   line = "";
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = "  ";
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = "a";
   expected.push_back("a");
   check(line,StringSplitter(line),expected);

   // Some implementation fail this test
   expected.clear();
   line = "\n";
   expected.push_back("\n");
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = "a ";
   expected.push_back("a");
   check(line,StringSplitter(line),expected);

   expected.clear(); expected.push_back("a");
   line = " a";
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = " a"; // check tabs
   expected.push_back("a");
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = "  a  "; // check sequential tabs
   expected.push_back("a");
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = " a ";
   expected.push_back("a");
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = "        a     b     c       d        ";
   expected.push_back("a"); expected.push_back("b"); expected.push_back("c"); expected.push_back("d");
   check(line,StringSplitter(line),expected);

   expected.clear();
   line = " - !   $ % ^  & * ( ) - + ? ";
   expected.push_back("-"); expected.push_back("!");  expected.push_back("$");
   expected.push_back("%"); expected.push_back("^"); expected.push_back("&"); expected.push_back("*");
   expected.push_back("("); expected.push_back(")"); expected.push_back("-"); expected.push_back("+");
   expected.push_back("?");
   check(line,StringSplitter(line),expected);

   // Check tabs
   expected.clear();
   line = "     verify complete:8                      # 4 sundays in october hence expect 8 task completions";
   expected.push_back("verify");expected.push_back("complete:8");expected.push_back("#");expected.push_back("4");
   expected.push_back("sundays");expected.push_back("in");expected.push_back("october");expected.push_back("hence");
   expected.push_back("expect");expected.push_back("8");expected.push_back("task");expected.push_back("completions");
   check(line,StringSplitter(line),expected);

   // Check paths
   expected.clear(); expected.push_back("a");
   line = "/a";
   check(line,StringSplitter(line,"/"),expected);

   expected.clear();
   line = "";
   check(line,StringSplitter(line,"/"),expected);

   expected.clear();
   expected.push_back("a");expected.push_back("b");expected.push_back("c");expected.push_back("c");expected.push_back("e");
   line = "/a/b/c/c//e";
   check(line,StringSplitter(line,"/"),expected);

   expected.clear();
   expected.push_back("a");expected.push_back("b");expected.push_back("c");expected.push_back("c");expected.push_back("e");
   line = "///a/b/c/c//e";
   check(line,StringSplitter(line,"/"),expected);

   expected.clear();
   expected.push_back("a");expected.push_back("b");expected.push_back("c");expected.push_back("c");expected.push_back("e");
   line = "//a/b/c/c//e/";
   check(line,StringSplitter(line,"/"),expected);

   expected.clear();
   expected.push_back("a ");expected.push_back("b");expected.push_back("c");expected.push_back("c e");
   line = "/a /b/c/c e";
   check(line,StringSplitter(line,"/"),expected);
}

BOOST_AUTO_TEST_SUITE_END()
