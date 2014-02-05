//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
//
// Copyright 2009-2012 ECMWF. 
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
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/split.hpp>

#include "Str.hpp"

using namespace std;
using namespace ecf;
using namespace boost;

//#define STRING_SPLIT_IMPLEMENTATIONS_PERF_CHECK_ 1;


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
}

static void check(const std::string& line,
                  const std::vector<std::string>& result,
                  const std::vector<std::string>& expected )
{
 	BOOST_CHECK_MESSAGE(result.size() == expected.size(),"expected size " << expected.size() << " but found " << result.size() << " for '" << line << "'");
	BOOST_CHECK_MESSAGE(result == expected,"Str::split failed for '" << line  << "'");
	if (result != expected) {
		cout << "Actual  :"; BOOST_FOREACH(const string& t, result)   { cout << "'" << t << "'"; } cout << "\n";
		cout << "Expected:"; BOOST_FOREACH(const string& t, expected) { cout << "'" << t << "'"; } cout << "\n";
 	}
}

//BOOST_AUTO_TEST_CASE( test_boost_str_split )
//{
//	cout << "ACore:: ...test_boost_str_split\n";
//
//	std::string line = "This is a string  ";
//	std::vector<std::string> expected;
//	expected.push_back("This"); expected.push_back("is"); expected.push_back("a"); expected.push_back("string");
//	expected.push_back(""); expected.push_back("");
//
//	std::vector<std::string> result;
//	boost::algorithm::split(result, line, std::bind2nd(std::equal_to<char>(), ' ')); // default is compress off, preserve empty tokens
//	check(line,result,expected);
//
//	expected.pop_back();
//	result.clear();
//	boost::algorithm::split(result, line, std::bind2nd(std::equal_to<char>(), ' '),boost::algorithm::token_compress_on);
//	check(line,result,expected);
//
//	// boost::split(v, s, boost::lambda::_1 == ' ');
//}


BOOST_AUTO_TEST_CASE( test_str_split )
{
	cout << "ACore:: ...test_str_split\n";

	std::string line = "This is a string";
	std::vector<std::string> expected;
	expected.push_back("This"); expected.push_back("is"); expected.push_back("a"); expected.push_back("string");
	std::vector<std::string> result;
	Str::split(line,result);
	check(line,result,expected);


	line.clear(); expected.clear(); result.clear();
	line = "  ";
 	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = "a";
	expected.push_back("a");
 	Str::split(line,result);
	check(line,result,expected);

	// Some implementation fail this test
	line.clear(); expected.clear(); result.clear();
	line = "\n";
	expected.push_back("\n");
  	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = "a ";
	expected.push_back("a");
 	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = " a";
	expected.push_back("a");
 	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = "	a"; // check tabs
	expected.push_back("a");
 	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = "		a		"; // check sequential tabs
	expected.push_back("a");
 	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = " a ";
	expected.push_back("a");
 	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = "        a     b     c       d        ";
	expected.push_back("a"); expected.push_back("b"); expected.push_back("c"); expected.push_back("d");
  	Str::split(line,result);
	check(line,result,expected);

	line.clear(); expected.clear(); result.clear();
	line = " - !   $ % ^ & * ( ) - + ?";
	expected.push_back("-"); expected.push_back("!");  expected.push_back("$");
	expected.push_back("%"); expected.push_back("^"); expected.push_back("&"); expected.push_back("*");
	expected.push_back("("); expected.push_back(")"); expected.push_back("-"); expected.push_back("+");
	expected.push_back("?");
  	Str::split(line,result);
	check(line,result,expected);

	// Check tabs
	line.clear(); expected.clear(); result.clear();
	line = "		 verify complete:8		                # 4 sundays in october hence expect 8 task completions";
	expected.push_back("verify");expected.push_back("complete:8");expected.push_back("#");expected.push_back("4");
	expected.push_back("sundays");expected.push_back("in");expected.push_back("october");expected.push_back("hence");
	expected.push_back("expect");expected.push_back("8");expected.push_back("task");expected.push_back("completions");
   	Str::split(line,result);
	check(line,result,expected);

#ifdef STRING_SPLIT_IMPLEMENTATIONS_PERF_CHECK_
 	{
 		line = "This is a long string that is going to be used to test the performance of splitting with different Implementations   extra   empty tokens   ";
		size_t times = 1000000;
	 	boost::timer timer;  // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
 		for (size_t i = 0; i < times; i++) {
 			result.clear();
// 			boost::algorithm::split(result, line, std::bind2nd(std::equal_to<char>(), ' '),boost::algorithm::token_compress_on); //  3.2 times slower, but preserves empty tokens
		  	Str::split(line,result);
 		}
		cout << "Time for Str::split " << times << " times = " << timer.elapsed() << "\n";
	}
#endif
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
   test_replace_all(testStr,"\n","\\n","This\\n is\\n a\\n string\\n");

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
   test_replace_all(testStr,"\n","\\n","This\\n is\\n a\\n string\\n");

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
//
//BOOST_AUTO_TEST_CASE( test_loop )
//{
//	// DEBUG release shows BOOST_FOREACH has worst perf, however in release mode its par with the fastest.
//	size_t vecSize = 20000000;
//	std::vector<Fred> vec;
//	vec.reserve(vecSize);
//	for (size_t i = 0; i < vecSize ; i++) { vec.push_back(Fred(i));}
//
// 	boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//	BOOST_FOREACH(Fred& fred, vec) { fred.inc(); }
// 	cout << "Time: BOOST_FOREACH(Fred& fred, vec) { fred.inc(); }                                       " << timer.elapsed() << "\n";
//
// 	timer.restart();
//	std::for_each(vec.begin(),vec.end(),boost::bind(&Fred::inc,_1) );
// 	cout << "Time: std::for_each(vec.begin(),vec.end(),boost::bind(&Fred::inc,_1) );                    " << timer.elapsed() << "\n";
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
//	if ( str.find_first_of( Str::NUMERIC(), 0 ) != std::string::npos ) {
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
//	if ( str.find_first_of( Str::NUMERIC(), 0 ) != std::string::npos ) {
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

BOOST_AUTO_TEST_SUITE_END()
