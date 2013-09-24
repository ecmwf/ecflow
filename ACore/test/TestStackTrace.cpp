////============================================================================
//// Name        :
//// Author      : Avi
//// Revision    : $Revision: #8 $ 
////
//// Copyright 2009-2012 ECMWF. 
//// This software is licensed under the terms of the Apache Licence version 2.0 
//// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
//// In applying this licence, ECMWF does not waive the privileges and immunities 
//// granted to it by virtue of its status as an intergovernmental organisation 
//// nor does it submit to any jurisdiction. 
////
//// Description :
////============================================================================
//#include <boost/test/unit_test.hpp>
//#include "boost/filesystem/operations.hpp"
//#include "boost/filesystem/path.hpp"
//#include <iostream>
//#include <fstream>
////#include "StackTrace.hpp"
//
//
//using namespace boost;
//using namespace std;
//using namespace ecf;
//namespace fs = boost::filesystem;
//
//BOOST_AUTO_TEST_SUITE( CoreTestSuite )
//
//class MyClass {
//public:
//    std::string MemFunc( const std::string &someParam )
//    {
//    	int depth = 5;
//        return StackTrace::dump( __FILE__, __LINE__, depth );
//    }
//};
//
//std::string func2( const char *something )
//{
//    MyClass a;
//    return a.MemFunc( something );
//}
//
//std::string func1( int param1, const std::string &param2 )
//{
//    return func2( param2.c_str() );
//}
//
////BOOST_AUTO_TEST_CASE( test_stack_trace )
////{
////#if defined(__GNUC__)
////	cout << "ACore:: ...test_stack_trace\n";
////    std::string traceback = func1( 1, "TestString" );
////
////	std::string expected;
////	fs::path current_path = fs::current_path();
////	if (current_path.stem() == "ACore" ) {
//////		cout << "current_path.stem() == ACore )\n";
////		expected = "Call Stack from test/TestStackTrace.cpp:28\n"
////		       "   CoreTestSuite::MyClass::MemFunc(std::string const&)\n"
////		       "   CoreTestSuite::func2(char const*)\n"
////		       "   CoreTestSuite::func1(int, std::string const&)\n"
////		       "   CoreTestSuite::test_stack_trace::test_method()\n";
////	}
////	else {
//////		cout << "current_path.stem() != ACore )\n";
////		expected = "Call Stack from ACore/test/TestStackTrace.cpp:28\n"
////		       "   CoreTestSuite::MyClass::MemFunc(std::string const&)\n"
////		       "   CoreTestSuite::func2(char const*)\n"
////		       "   CoreTestSuite::func1(int, std::string const&)\n"
////		       "   CoreTestSuite::test_stack_trace::test_method()\n";
////	}
////    BOOST_CHECK_MESSAGE (traceback == expected,"Mismatch expected\n'" << expected << "'\n but found\n'" << traceback << "'");
////#endif
////}
//
//BOOST_AUTO_TEST_SUITE_END()
//
//
