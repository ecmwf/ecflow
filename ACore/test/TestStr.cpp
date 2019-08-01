//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>

#include "Str.hpp"
#include "StringSplitter.hpp"

using namespace std;
using namespace ecf;
using namespace boost;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_str )
{
	cout << "ACore:: ...test_str\n";

	{
		std::string str;
		std::string expected;
		Str::removeQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);

		str = "\"\""; expected = "";
		Str::removeQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);

		str = "fred"; expected = "fred";
		Str::removeQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);

		str = "\"fred\""; expected = "fred";
		Str::removeQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);
	}
	{
		std::string str;
		std::string expected;
		Str::removeSingleQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);

		str = "''"; expected = "";
		Str::removeSingleQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);

		str = "fred"; expected = "fred";
		Str::removeSingleQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);

		str = "'fred'"; expected = "fred";
		Str::removeSingleQuotes(str);
		BOOST_CHECK_MESSAGE(str == expected," Expected " << expected << " but found " << str);
	}
	{
		string test;
		BOOST_CHECK_MESSAGE(!Str::truncate_at_start(test,7),"Empty sring should return false");

		test= "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
		string expected = "line";
		BOOST_CHECK_MESSAGE(Str::truncate_at_start(test,1) && test==expected,"Expected:\n" << expected << "\nbut found:\n" << test);

		test= "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
		expected = "a\nstring\nwith\nlots\nof\nnew\nline";
		BOOST_CHECK_MESSAGE(Str::truncate_at_start(test,7) && test==expected,"Expected:\n" << expected << "\nbut found:\n" << test);

		test= "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
		expected = test;
		BOOST_CHECK_MESSAGE(!Str::truncate_at_start(test,9) && test==expected,"Expected:\n" << expected << "\nbut found:\n" << test);
	}
   {
      string test;
      BOOST_CHECK_MESSAGE(!Str::truncate_at_end(test,7),"Empty string should return false");

      test= "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
      string expected = "this\n";
      BOOST_CHECK_MESSAGE(Str::truncate_at_end(test,1) && test==expected,"Expected:\n" << expected << "\nbut found:\n" << test);

      test= "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
      expected = "this\nis\n";
      BOOST_CHECK_MESSAGE(Str::truncate_at_end(test,2) && test==expected,"Expected:\n" << expected << "\nbut found:\n" << test);

      test= "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
      expected= "this\nis\na\nstring\nwith\nlots\nof\n";
      BOOST_CHECK_MESSAGE(Str::truncate_at_end(test,7) && test==expected,"Expected:\n" << expected << "\nbut found:\n" << test);

      test= "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
      expected = test;
      BOOST_CHECK_MESSAGE(!Str::truncate_at_end(test,9) && test==expected,"Expected:\n" << expected << "\nbut found:\n" << test);
   }
}

static void check(const std::string& line,
                  const std::vector<std::string>& result,
                  const std::vector<std::string>& expected )
{
 	BOOST_CHECK_MESSAGE(result.size() == expected.size(),"expected size " << expected.size() << " but found " << result.size() << " for '" << line << "'");
	BOOST_CHECK_MESSAGE(result == expected,"failed for '" << line  << "'");
	if (result != expected) {
	   cout << "Line    :'" << line << "'\n";
		cout << "Actual  :"; BOOST_FOREACH(const string& t, result)   { cout << "'" << t << "'"; } cout << "\n";
		cout << "Expected:"; BOOST_FOREACH(const string& t, expected) { cout << "'" << t << "'"; } cout << "\n";
 	}
}

static void check(const std::string& line,
                  const std::vector<boost::string_view>& result2,
                  const std::vector<std::string>& expected )
{
   std::vector<std::string> result;
   for(auto ref : result2) {
      result.emplace_back(ref.begin(),ref.end());
   }
   check(line,result,expected);
}

static void check(const std::string& line,
                  boost::split_iterator<std::string::const_iterator> res,
                  const std::vector<std::string>& expected )
{
   std::vector<std::string> result;
   typedef boost::split_iterator<std::string::const_iterator> split_iter_t;
   for(; res!= split_iter_t(); res++)  result.push_back(boost::copy_range<std::string>(*res));
   check(line,result,expected);
}

static void check_splitters(const std::string& line,const std::vector<std::string>& expected)
{
   std::vector<std::string> result,result1,result2,result3;
   std::vector<boost::string_view> result4;

   Str::split_orig(line,result);               check(line,result,expected);
   Str::split_orig1(line,result1);             check(line,result1,expected);
   Str::split_using_string_view(line,result2); check(line,result2,expected);
   Str::split_using_string_view2(line,result3);check(line,result3,expected);
   StringSplitter::split(line,result4);        check(line,result4,expected);

   // While were at it also check get_token
   for(size_t i=0; i < result1.size(); i++){
      std::string token;
      BOOST_CHECK_MESSAGE(Str::get_token(line,i,token) && token == result1[i],"Str::get_token failed for pos " << i << " line:'" << line << "' expected '" <<  result1[i] << "' but found '" << token << "'");

      token.clear();
      BOOST_CHECK_MESSAGE(Str::get_token2(line,i,token) && token == result1[i],"Str::get_token2 failed for pos " << i << " line:'" << line << "' expected '" <<  result1[i] << "' but found '" << token << "'");

      token.clear();
      BOOST_CHECK_MESSAGE(Str::get_token3(line,i,token) && token == result1[i],"Str::get_token3 failed for pos " << i << " line:'" << line << "' expected '" <<  result1[i] << "' but found '" << token << "'");

      token.clear();
      BOOST_CHECK_MESSAGE(StringSplitter::get_token(line,i,token) && token == result1[i],"StringSplitter::get_token failed for pos " << i << " line:'" << line << "' expected '" <<  result1[i] << "' but found '" << token << "'");
   }
}

BOOST_AUTO_TEST_CASE( test_str_split )
{
	cout << "ACore:: ...test_str_split\n";

	std::vector<std::string> expected;

	std::string line = "This is a string";
	expected.emplace_back("This"); expected.emplace_back("is"); expected.emplace_back("a"); expected.emplace_back("string");
	check_splitters(line,expected);

	expected.clear();
	line = "  ";
   check_splitters(line,expected);

	line = "a";
   expected.clear(); expected.emplace_back("a");
   check_splitters(line,expected);

	// Some implementation fail this test
	line = "\n";
   expected.clear(); expected.emplace_back("\n");
   check_splitters(line,expected);

	line = "a ";
   expected.clear(); expected.emplace_back("a");
   check_splitters(line,expected);

	line = " a";
   expected.clear(); expected.emplace_back("a");
   check_splitters(line,expected);

	line = "	a"; // check tabs
   expected.clear(); expected.emplace_back("a");
   check_splitters(line,expected);

	line = "		a		"; // check sequential tabs
   expected.clear(); expected.emplace_back("a");
   check_splitters(line,expected);

	line = " a ";
   expected.clear();  expected.emplace_back("a");
   check_splitters(line,expected);

	line = "        a     b     c       d        ";
   expected.clear(); expected.emplace_back("a"); expected.emplace_back("b"); expected.emplace_back("c"); expected.emplace_back("d");
   check_splitters(line,expected);

	line = " - !   $ % ^ & * ( ) - + ?";
   expected.clear();
	expected.emplace_back("-"); expected.emplace_back("!");  expected.emplace_back("$");
	expected.emplace_back("%"); expected.emplace_back("^"); expected.emplace_back("&"); expected.emplace_back("*");
	expected.emplace_back("("); expected.emplace_back(")"); expected.emplace_back("-"); expected.emplace_back("+");
	expected.emplace_back("?");
   check_splitters(line,expected);

	// Check tabs
	line = "		 verify complete:8		                # 4 sundays in october hence expect 8 task completions";
   expected.clear();
	expected.emplace_back("verify");expected.emplace_back("complete:8");expected.emplace_back("#");expected.emplace_back("4");
	expected.emplace_back("sundays");expected.emplace_back("in");expected.emplace_back("october");expected.emplace_back("hence");
	expected.emplace_back("expect");expected.emplace_back("8");expected.emplace_back("task");expected.emplace_back("completions");
   check_splitters(line,expected);
}


BOOST_AUTO_TEST_CASE( test_str_split_make_split_iterator )
{
   cout << "ACore:: ...test_str_split_make_split_iterator\n";

   std::string line = "This is a string";
   std::vector<std::string> expected;
   expected.emplace_back("This"); expected.emplace_back("is"); expected.emplace_back("a"); expected.emplace_back("string");
   check(line,Str::make_split_iterator(line),expected);

   expected.clear(); expected.emplace_back("");
   line = "";
   check(line,Str::make_split_iterator(line),expected);

   expected.clear(); expected.emplace_back(""); expected.emplace_back("");
   line = "  ";                                  // If start/end is delimeter, then preserved as empty token
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = "a";
   expected.emplace_back("a");
   check(line,Str::make_split_iterator(line),expected);

   // Some implementation fail this test
   expected.clear();
   line = "\n";
   expected.emplace_back("\n");
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = "a ";
   expected.emplace_back("a");expected.emplace_back(""); // delimeter at start/end preserved, as empty token
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = " a";
   expected.emplace_back(""); expected.emplace_back("a"); // delimeter at start/end preserved, as empty token
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = " a"; // check tabs
   expected.emplace_back(""); expected.emplace_back("a");  // delimeter at start/end preserved, as empty token
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = "    a     "; // check sequential tabs
   expected.emplace_back("");
   expected.emplace_back("a");                        // delimeter at start/end preserved, as empty token
   expected.emplace_back("");
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = " a ";
   expected.emplace_back("");
   expected.emplace_back("a");                        // delimeter at start/end preserved, as empty token
   expected.emplace_back("");
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = "        a     b     c       d        ";
   expected.emplace_back("");                         // delimeter at start/end preserved, as empty token
   expected.emplace_back("a"); expected.emplace_back("b"); expected.emplace_back("c"); expected.emplace_back("d");
   expected.emplace_back("");
   check(line,Str::make_split_iterator(line),expected);

   expected.clear();
   line = " - !   $ % ^  & * ( ) - + ?";
   expected.emplace_back("");                         // delimeter at start/end preserved, as empty token
   expected.emplace_back("-"); expected.emplace_back("!");  expected.emplace_back("$");
   expected.emplace_back("%"); expected.emplace_back("^"); expected.emplace_back("&"); expected.emplace_back("*");
   expected.emplace_back("("); expected.emplace_back(")"); expected.emplace_back("-"); expected.emplace_back("+");
   expected.emplace_back("?");
   check(line,Str::make_split_iterator(line),expected);

   // Check tabs
   expected.clear();
   line = "     verify complete:8                      # 4 sundays in october hence expect 8 task completions";
   expected.emplace_back("");                         // delimeter at start/end preserved, as empty token
   expected.emplace_back("verify");expected.emplace_back("complete:8");expected.emplace_back("#");expected.emplace_back("4");
   expected.emplace_back("sundays");expected.emplace_back("in");expected.emplace_back("october");expected.emplace_back("hence");
   expected.emplace_back("expect");expected.emplace_back("8");expected.emplace_back("task");expected.emplace_back("completions");
   check(line,Str::make_split_iterator(line),expected);
}

static void test_replace( std::string& testStr, const std::string& find, const std::string& replace, const std::string& expected)
{
 	BOOST_CHECK_MESSAGE(Str::replace(testStr,find,replace), "Replace failed for " << testStr << " find(" << find << ") replace(" << replace << ")");
	BOOST_CHECK_MESSAGE(testStr == expected,"Expected '" << expected << "' but found '" << testStr <<"'");
}

static void test_replace_all( std::string& testStr, const std::string& find, const std::string& replace, const std::string& expected)
{
   std::string testStrCopy = testStr;

   BOOST_CHECK_MESSAGE(Str::replace_all(testStr,find,replace), "Replace failed for " << testStr << " find(" << find << ") replace(" << replace << ")");
   BOOST_CHECK_MESSAGE(testStr == expected,"Expected '" << expected << "' but found '" << testStr <<"'");

   Str::replaceall(testStrCopy,find,replace);
   BOOST_CHECK_MESSAGE(testStr == testStrCopy,"Expected '" << testStrCopy << "' but found '" << testStr <<"'");
}


BOOST_AUTO_TEST_CASE( test_str_replace )
{
	cout << "ACore:: ...test_str_replace\n";

	std::string testStr = "This is a string";
 	test_replace(testStr,"This","That","That is a string");

	testStr = "This is a string";
 	test_replace(testStr,"This is a string","","");

	testStr = "This is a string";
 	test_replace(testStr,"is a","was a","This was a string");

   testStr = "This\n is a string";
   test_replace(testStr,"\n","\\n","This\\n is a string");

   testStr = "This\n is\n a\n string\n";
   test_replace_all(testStr,"\n","\\n",R"(This\n is\n a\n string\n)");

 	// Test case insenstive string comparison
	BOOST_CHECK_MESSAGE(Str::caseInsCompare("","")," bug1");
	BOOST_CHECK_MESSAGE(!Str::caseInsCompare("Str","Str1")," bug1");
	BOOST_CHECK_MESSAGE(!Str::caseInsCompare("","Str1")," bug1");
 	BOOST_CHECK_MESSAGE(Str::caseInsCompare("Str","STR")," bug1");
 	BOOST_CHECK_MESSAGE(Str::caseInsCompare("Case","CaSE")," bug1");
}

BOOST_AUTO_TEST_CASE( test_str_replace_all )
{
   cout << "ACore:: ...test_str_replace_all\n";

   std::string testStr = "This is a string";
   test_replace_all(testStr,"This","That","That is a string");

   testStr = "This is a string";
   test_replace_all(testStr,"This is a string","","");

   testStr = "This is a string";
   test_replace_all(testStr,"is a","was a","This was a string");

   testStr = "This\n is a string";
   test_replace_all(testStr,"\n","\\n","This\\n is a string");

   testStr = "This\n is\n a\n string\n";
   test_replace_all(testStr,"\n","\\n",R"(This\n is\n a\n string\n)");

   testStr = "This\n is\n a\n string\n";
   test_replace_all(testStr,"\n","","This is a string");
}

BOOST_AUTO_TEST_CASE( test_str_to_int )
{
	cout << "ACore:: ...test_str(to_int)\n";
	BOOST_CHECK_MESSAGE(Str::to_int("0") == 0,"Expected 0");
	BOOST_CHECK_MESSAGE(Str::to_int("1") == 1,"Expected 1");
	BOOST_CHECK_MESSAGE(Str::to_int("-0") == 0,"Expected 0");
	BOOST_CHECK_MESSAGE(Str::to_int("-1") == -1,"Expected -1");
	BOOST_CHECK_MESSAGE(Str::to_int("") == std::numeric_limits<int>::max(),"Expected max int");
	BOOST_CHECK_MESSAGE(Str::to_int("-") == std::numeric_limits<int>::max(),"Expected max int");
	BOOST_CHECK_MESSAGE(Str::to_int(" ") == std::numeric_limits<int>::max(),"Expected max int");
	BOOST_CHECK_MESSAGE(Str::to_int("q") == std::numeric_limits<int>::max(),"Expected max int");
	BOOST_CHECK_MESSAGE(Str::to_int("q22") == std::numeric_limits<int>::max(),"Expected max int");
	BOOST_CHECK_MESSAGE(Str::to_int("q22",-1) == -1,"Expected -1 on failure");
	BOOST_CHECK_MESSAGE(Str::to_int("99 99") == std::numeric_limits<int>::max(),"Expected max int");
	BOOST_CHECK_MESSAGE(Str::to_int("99 99",0) == 0,"Expected 0 for failure");
}

BOOST_AUTO_TEST_CASE( test_extract_data_member_value )
{
   cout << "ACore:: ...test_extract_data_member_value\n";
   std::string expected = "value";
   std::string actual;
   std::string str = "aa bb c fred:value";
   BOOST_CHECK_MESSAGE(Str::extract_data_member_value(str,"fred:",actual)," failed");
   BOOST_CHECK_MESSAGE(expected == actual,"expected '" << expected << "' but found '" << actual << "'");


   str = "fred:x  bill:zzz jake:12345 1234:99  6677";
   expected = "x";
   BOOST_CHECK_MESSAGE(Str::extract_data_member_value(str,"fred:",actual)," failed");
   BOOST_CHECK_MESSAGE(expected == actual,"expected '" << expected << "' but found '" << actual << "'");

   expected = "zzz";
   BOOST_CHECK_MESSAGE(Str::extract_data_member_value(str,"bill:",actual)," failed");
   BOOST_CHECK_MESSAGE(expected == actual,"expected '" << expected << "' but found '" << actual << "'");

   expected = "12345";
   BOOST_CHECK_MESSAGE(Str::extract_data_member_value(str,"jake:",actual)," failed");
   BOOST_CHECK_MESSAGE(expected == actual,"expected '" << expected << "' but found '" << actual << "'");

   expected = "99";
   BOOST_CHECK_MESSAGE(Str::extract_data_member_value(str,"1234:",actual)," failed");
   BOOST_CHECK_MESSAGE(expected == actual,"expected '" << expected << "' but found '" << actual << "'");

   expected = "77";
   BOOST_CHECK_MESSAGE(Str::extract_data_member_value(str,"66",actual)," failed");
   BOOST_CHECK_MESSAGE(expected == actual,"expected '" << expected << "' but found '" << actual << "'");
}


std::string toString(const std::vector<std::string>& c)
{
   std::stringstream ss;
   std::copy (c.begin(), c.end(), std::ostream_iterator <std::string> (ss, ", "));
   return ss.str();
}

BOOST_AUTO_TEST_CASE( test_str_less_greater)
{
   cout << "ACore:: ...test_str_less_greater\n";

   std::vector<std::string> expected;
   expected.emplace_back("a1");
   expected.emplace_back("A2");
   expected.emplace_back("b1");
   expected.emplace_back("B2");
   expected.emplace_back("c");

   std::vector<std::string> expectedGreater;
   expectedGreater.emplace_back("c");
   expectedGreater.emplace_back("B2");
   expectedGreater.emplace_back("b1");
   expectedGreater.emplace_back("A2");
   expectedGreater.emplace_back("a1");

   std::vector<std::string> vec;
   vec.emplace_back("c");
   vec.emplace_back("A2");
   vec.emplace_back("a1");
   vec.emplace_back("b1");
   vec.emplace_back("B2");

   std::sort(vec.begin(),vec.end(),Str::caseInsLess);
   BOOST_REQUIRE_MESSAGE( vec == expected,"expected " << toString(expected) << " but found " << toString(vec) );

   std::sort(vec.begin(),vec.end(),Str::caseInsGreater);
   BOOST_REQUIRE_MESSAGE( vec == expectedGreater,"expected " << toString(expectedGreater) << " but found " << toString(vec) );

   // --------------------------------------------------------------------

   expected.clear();
   expected.emplace_back("a");
   expected.emplace_back("A");
   expected.emplace_back("b");
   expected.emplace_back("B");
   expected.emplace_back("c");

   expectedGreater.clear();
   expectedGreater.emplace_back("c");
   expectedGreater.emplace_back("B");
   expectedGreater.emplace_back("b");
   expectedGreater.emplace_back("A");
   expectedGreater.emplace_back("a");

   vec.clear();
   vec.emplace_back("c");
   vec.emplace_back("B");
   vec.emplace_back("A");
   vec.emplace_back("b");
   vec.emplace_back("a");

   std::sort(vec.begin(),vec.end(),Str::caseInsLess);
   BOOST_REQUIRE_MESSAGE( vec == expected,"expected " << toString(expected) << " but found " << toString(vec) );

   std::sort(vec.begin(),vec.end(),Str::caseInsGreater);
   BOOST_REQUIRE_MESSAGE( vec == expectedGreater,"expected " << toString(expectedGreater) << " but found " << toString(vec) );

   // --------------------------------------------------------------------

   expected.clear();
   expected.emplace_back("1234");
   expected.emplace_back("baSE");
   expected.emplace_back("Base");
   expected.emplace_back("case");
   expected.emplace_back("CaSe");
   expected.emplace_back("suite");
   expected.emplace_back("SUITE");

   expectedGreater.clear();
   expectedGreater.emplace_back("SUITE");
   expectedGreater.emplace_back("suite");
   expectedGreater.emplace_back("CaSe");
   expectedGreater.emplace_back("case");
   expectedGreater.emplace_back("Base");
   expectedGreater.emplace_back("baSE");
   expectedGreater.emplace_back("1234");

   vec.clear();
   vec.emplace_back("suite");
   vec.emplace_back("SUITE");
   vec.emplace_back("baSE");
   vec.emplace_back("Base");
   vec.emplace_back("case");
   vec.emplace_back("CaSe");
   vec.emplace_back("1234");

   std::sort(vec.begin(),vec.end(),Str::caseInsLess);
   BOOST_REQUIRE_MESSAGE( vec == expected,"expected " << toString(expected) << " but found " << toString(vec) );

   std::sort(vec.begin(),vec.end(),Str::caseInsGreater);
   BOOST_REQUIRE_MESSAGE( vec == expectedGreater,"expected " << toString(expectedGreater) << " but found " << toString(vec) );
}


//// ==============================================================
//// Timing to find the fastest looping
//// ==============================================================
//class Fred {
//public:
//   Fred(int i = 0) : i_(i)            { /*std::cout << "Fred constructor\n"*/;}
//   Fred(const Fred& rhs) : i_(rhs.i_) { /*std::cout << "Fred copy constructor\n";*/ }
//   Fred& operator=(const Fred& rhs)   { /*std::cout << "assignment operator\n";*/ i_ = rhs.i_; return *this;}
//   ~Fred()                            { /*std::cout << "Fred destructor\n";*/}
//
//	void inc() { i_++;}
//private:
//	int i_;
//};

//BOOST_AUTO_TEST_CASE( test_loop )
//{
//	// DEBUG release shows BOOST_FOREACH has worst perf, however in release mode its par with the fastest.
//	size_t vecSize = 200000000;
//	std::vector<Fred> vec;
//	vec.reserve(vecSize);
//	for (size_t i = 0; i < vecSize ; i++) { vec.push_back(Fred(i));}
//
// 	boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//	BOOST_FOREACH(Fred& fred, vec) { fred.inc(); }
// 	cout << "Time: BOOST_FOREACH(Fred& fred, vec) { fred.inc(); }                                       " << timer.elapsed() << "\n";
//
//   timer.restart();
//   for(auto &fred : vec) { fred.inc(); }
//   cout << "Time: for(auto &fred : vec) { fred.inc(); }                                                " << timer.elapsed() << "\n";
//
//   timer.restart();
//   std::for_each(vec.begin(),vec.end(),[](Fred& fred) { fred.inc();} );
//   cout << "Time: std::for_each(vec.begin(),vec.end(),[](Fred& fred) { fred.inc();} );                 " << timer.elapsed() << "\n";
//
// 	timer.restart();
//	std::vector<Fred>::iterator theEnd = vec.end();
// 	for (std::vector<Fred>::iterator  i = vec.begin(); i < theEnd ; i++) { (*i).inc(); }
// 	cout << "Time: for (std::vector<Fred>::iterator  i = vec.begin(); i < theEnd ; i++) { (*i).inc(); } " << timer.elapsed() << "\n";
//
// 	timer.restart();
// 	std::for_each(vec.begin(),vec.end(),std::mem_fun_ref(&Fred::inc) );
// 	cout << "Time: std::for_each(vec.begin();vec.end(),std::mem_fun_ref(&Fred::inc))                    " << timer.elapsed() << "\n";
//
// 	timer.restart();
// 	size_t theSize = vec.size();
//	for (size_t i = 0; i < theSize ; i++) { vec[i].inc(); }
// 	cout << "Time: for (size_t i = 0; i < theSize ; i++) { vec[i].inc(); }                              " << timer.elapsed() << "\n";
//}


/// ==============================================================
/// Timing to find the fastest conversion from string to int
/// ==============================================================
//static void methodX(  const std::string& str,
//                      std::vector<std::string>& stringRes,
//                      std::vector<int>& numberRes)
//{
//	// 0.81
//	// for bad conversion istringstream seems to return 0, hence add guard
//	if ( str.find_first_of( Str::NUMERIC(), 0 ) == 0 ) {
//		int number = 0;
//		std::istringstream ( str ) >> number;
//		numberRes.push_back( number );
// 	}
//	else {
//		stringRes.push_back( str );
//	}
//}
//
//
//static void method1(  const std::string& str,
//                      std::vector<std::string>& stringRes,
//                      std::vector<int>& numberRes)
//{
//	// 12.2
//	try {
//		int number = boost::lexical_cast< int >( str );
//		numberRes.push_back( number );
//	}
//	catch ( boost::bad_lexical_cast& ) {
//		stringRes.push_back( str );
//	}
//}
//
//static void method2(  const std::string& str,
//                      std::vector<std::string>& stringRes,
//                      std::vector<int>& numberRes)
//{
//	// 0.6
//	if ( str.find_first_of( Str::NUMERIC(), 0 ) == 0 ) {
//		try {
//			int number = boost::lexical_cast< int >( str );
//			numberRes.push_back( number );
//		}
//		catch ( boost::bad_lexical_cast& ) {
//			stringRes.push_back( str );
//		}
//	}
//	else {
//		stringRes.push_back( str );
//	}
//}
//
//static void method3(  const std::string& str,
//                      std::vector<std::string>& stringRes,
//                      std::vector<int>& numberRes)
//{
//	// 0.14
//	// atoi return 0 for errors,
//	int number = atoi(str.c_str()); //does not handle errors
//	if (number == 0 && str.size() != 1) {
//		stringRes.push_back( str );
//	}
//	else {
//		numberRes.push_back( number );
//	}
//}
//
//
//BOOST_AUTO_TEST_CASE( test_lexical_cast_perf )
//{
//	cout << "ACore:: ...test_string_to_int_conversion\n";
//
//	size_t the_size  = 1000000;
//	std::vector<std::string> stringTokens;
//	std::vector<std::string> numberTokens;
// 	std::vector<int> expectedNumberRes;
//	for(size_t i=0; i < the_size; i++) { stringTokens.push_back("astring");}
//	for(size_t i=0; i < the_size; i++) {
//		numberTokens.push_back(boost::lexical_cast<string>(i));
//		expectedNumberRes.push_back(i);
//   }
//
//	std::vector<std::string> stringRes; stringTokens.reserve(stringTokens.size());
//	std::vector<int> numberRes; numberRes.reserve(expectedNumberRes.size());
//
//	{
//		boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//		for(size_t i =0; i < stringTokens.size(); i++) {
//			method1(stringTokens[i], stringRes, numberRes );
//		}
//		for(size_t i =0; i < numberTokens.size(); i++) {
//			method1(numberTokens[i], stringRes, numberRes );
//		}
//		cout << "Time for method1  elapsed time = " << timer.elapsed() << "\n";
//		BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes," method 1 wrong");
//		BOOST_CHECK_MESSAGE(stringTokens == stringRes,"method 1 wrong");
//		numberRes.clear();
//		stringRes.clear();
//	}
//
//	{
//		boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//		for(size_t i =0; i < stringTokens.size(); i++) {
//			methodX(stringTokens[i], stringRes, numberRes );
//		}
//		for(size_t i =0; i < numberTokens.size(); i++) {
//			methodX(numberTokens[i], stringRes, numberRes );
//		}
//		cout << "Time for methodX  elapsed time = " << timer.elapsed() << "\n";
//		BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes," method X wrong");
//		BOOST_CHECK_MESSAGE(stringTokens == stringRes,"method X wrong");
//		numberRes.clear();
//		stringRes.clear();
//	}
//
//	{
//		boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//		for(size_t i =0; i < stringTokens.size(); i++) {
//			method2(stringTokens[i], stringRes, numberRes );
//		}
//		for(size_t i =0; i < numberTokens.size(); i++) {
//			method2(numberTokens[i], stringRes, numberRes );
//		}
//		cout << "Time for method2  elapsed time = " << timer.elapsed() << "\n";
//		BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes,"method 2 wrong");
//		BOOST_CHECK_MESSAGE(stringTokens == stringRes,"method 2 wrong");
//		numberRes.clear();
//		stringRes.clear();
//	}
//
//	{
//		boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//		for(size_t i =0; i < stringTokens.size(); i++) {
//			method3(stringTokens[i], stringRes, numberRes );
//		}
//		for(size_t i =0; i < numberTokens.size(); i++) {
//			method3(numberTokens[i], stringRes, numberRes );
//		}
//		cout << "Time for method3  elapsed time = " << timer.elapsed() << "\n";
//		BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes," method3 wrong  numberRes.size()=" << numberRes.size() << " expected size = " << expectedNumberRes.size());
//		BOOST_CHECK_MESSAGE(stringTokens == stringRes," method3 wrong  stringRes.size()=" << stringRes.size() << " expected size = " << stringTokens.size());
//		numberRes.clear();
//		stringRes.clear();
//	}
//}

//BOOST_AUTO_TEST_CASE( test_int_to_str_perf )
//{
//   cout << "ACore:: ...test_int_to_str_perf\n";
//
//   // Lexical_cast is approx twice as fast as using streams
//   // time for ostream = 0.97
//   // time for lexical_cast = 0.45
//
//   const int the_size = 1000000;
//   {
//      boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//      for(size_t i =0; i < the_size; i++) {
//         std::ostringstream st;
//         st << i;
//         std::string s = st.str();
//      }
//      cout << "Time for int to string using ostringstream  elapsed time = " << timer.elapsed() << "\n";
//   }
//
//
//   {
//      boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//      for(size_t i =0; i < the_size; i++) {
//         std::string s = boost::lexical_cast<std::string>(i);
//      }
//      cout << "Time for int to string using boost::lexical_cast  elapsed time = " << timer.elapsed() << "\n";
//   }
//}


BOOST_AUTO_TEST_CASE( test_str_valid_name )
{
   cout << "ACore:: ...test_str_valid_name\n";

   std::vector<std::string> valid;
   valid.push_back("a");
   valid.push_back("a122345");
   valid.push_back("_a122345");
   valid.push_back("_");
   valid.push_back("0");
   valid.push_back("1");
   valid.push_back("2");
   valid.push_back("3");
   valid.push_back("4");
   valid.push_back("5");
   valid.push_back("6");
   valid.push_back("7");
   valid.push_back("8");
   valid.push_back("9");
   valid.push_back("11");
   valid.push_back("111");
   for(const auto & i : valid) {
      std::string msg;
      BOOST_CHECK_MESSAGE( Str::valid_name( i,msg ) ,"Expected " <<  i << " to be valid" );
      BOOST_CHECK_MESSAGE( Str::valid_name( i) ,"Expected " <<  i << " to be valid" );
   }

   BOOST_CHECK_MESSAGE( !Str::valid_name( "") ,"Expected empty string to be in-valid" );
   BOOST_CHECK_MESSAGE( !Str::valid_name( ".") ,"Expected '.' string to be in-valid" );
   std::vector<std::string> invalid;
   invalid.push_back("?");
   invalid.push_back("!");
   invalid.push_back("\"");
   invalid.push_back("$");
   invalid.push_back("%");
   invalid.push_back("^");
   invalid.push_back("*");
   invalid.push_back("(");
   invalid.push_back(")");
   invalid.push_back("-");
   invalid.push_back("+");
   invalid.push_back(":");
   invalid.push_back(";");
   invalid.push_back("@");
   invalid.push_back("~");
   invalid.push_back("<");
   invalid.push_back(">");
   invalid.push_back("!");
   for(const auto & i : invalid) {
      std::string msg;
      BOOST_CHECK_MESSAGE( !Str::valid_name( i,msg ) ,"Expected " << i << " to be in-valid" );
      BOOST_CHECK_MESSAGE( !Str::valid_name( i) ,"Expected " << i << " to be in-valid" );

      std::string s = "a" + i;
      BOOST_CHECK_MESSAGE( !Str::valid_name( s,msg ) ,"Expected " <<  s << " to be in-valid" );
      BOOST_CHECK_MESSAGE( !Str::valid_name( s ) ,"Expected " <<  s << " to be in-valid" );
   }
}

BOOST_AUTO_TEST_SUITE_END()
