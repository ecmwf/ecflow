#define BOOST_TEST_MODULE TestCore
//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #7 $ 
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
#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/timer.hpp>

#include "NodePath.hpp"

using namespace std;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

static void checkPath( const std::vector<std::string>& expectedPath,  const std::string& path  )
{
 	std::vector<std::string> thePath;
	NodePath::split(path,thePath);
  	if ( thePath != expectedPath ) {
  		BOOST_CHECK_MESSAGE(false,"Failed for " << path );
  		std::cout << "Expected '";
 		std::copy (expectedPath.begin(), expectedPath.end(), std::ostream_iterator <std::string> (std::cout, " "));
		std::cout << "'\nbut found '";
 		std::copy (thePath.begin(), thePath.end(), std::ostream_iterator <std::string> (std::cout, " "));
 		std::cout << "'\n";
 	}
}

BOOST_AUTO_TEST_CASE( test_path_extractor_constructor )
{
	cout << "ACore:: ...test_path_extractor_constructor\n";
	BOOST_CHECK(true); // stop boost test from complaining about no checks

	std::vector<std::string> theExpectedPath;
	checkPath(theExpectedPath,"");

	theExpectedPath.push_back("suite");
	checkPath(theExpectedPath,"/suite");
 	checkPath(theExpectedPath,"suite");
}

BOOST_AUTO_TEST_CASE( test_path_extractor )
{
	cout << "ACore:: ...test_path_extractor\n";
	BOOST_CHECK(true); // stop boost test from complaining about no checks

	std::vector<std::string> theExpectedPath;
	theExpectedPath.push_back("suite");
	theExpectedPath.push_back("family");
	theExpectedPath.push_back("task");

	checkPath(theExpectedPath,"/suite/family/task");
	checkPath(theExpectedPath,"/suite/family/task/");
}

BOOST_AUTO_TEST_CASE( test_unix_path_extractor )
{
	cout << "ACore:: ...test_unix_path_extractor\n";
	BOOST_CHECK(true); // stop boost test from complaining about no checks

	// On Unix multiple '/' are treated as one.
	std::vector<std::string> theExpectedPath;
	theExpectedPath.push_back("suite");
	theExpectedPath.push_back("family");
	theExpectedPath.push_back("task");

	checkPath(theExpectedPath,"/suite///family////task");
	checkPath(theExpectedPath,"/suite///family////task//");
	checkPath(theExpectedPath,"//suite///family////task//");
	checkPath(theExpectedPath,"///suite///family////task//");
	checkPath(theExpectedPath,"///suite///family////task///");
}

BOOST_AUTO_TEST_CASE( test_extractHostPort )
{
	cout << "ACore:: ...test_extractHostPort\n";

	std::string path;
	std::string host;
	std::string port;
	BOOST_CHECK_MESSAGE(!NodePath::extractHostPort(path,host,port), "expected failure");

	path = "Apath";
	BOOST_CHECK_MESSAGE(!NodePath::extractHostPort(path,host,port), "expected failure");

	path = " : ";
	BOOST_CHECK_MESSAGE(!NodePath::extractHostPort(path,host,port), "expected failure");

	path = "host:";
	BOOST_CHECK_MESSAGE(!NodePath::extractHostPort(path,host,port), "expected failure");

	path = ":port";
	BOOST_CHECK_MESSAGE(!NodePath::extractHostPort(path,host,port), "expected failure");

	path = "host:port";
	BOOST_CHECK_MESSAGE(NodePath::extractHostPort(path,host,port), "expected success " << host << ":" << port);
	BOOST_CHECK_MESSAGE(host == "host" && port == "port", "expected 'host:port' found " << host << ":" << port);

	path = "//host:port";
	BOOST_CHECK_MESSAGE(NodePath::extractHostPort(path,host,port), "expected success " << host << ":" << port);
	BOOST_CHECK_MESSAGE(host == "host" && port == "port", "expected 'host:port' found " << host << ":" << port);

	path = "//host:port/";
	BOOST_CHECK_MESSAGE(NodePath::extractHostPort(path,host,port), "expected success " << host << ":" << port);
	BOOST_CHECK_MESSAGE(host == "host" && port == "port", "expected 'host:port' found " << host << ":" << port);

	path = "//host:port/suite";
	BOOST_CHECK_MESSAGE(NodePath::extractHostPort(path,host,port), "expected success " << host << ":" << port);
	BOOST_CHECK_MESSAGE(host == "host" && port == "port", "expected 'host:port' found " << host << ":" << port);

	path = "//host:port/suite/family/task";
	BOOST_CHECK_MESSAGE(NodePath::extractHostPort(path,host,port), "expected success " << host << ":" << port);
	BOOST_CHECK_MESSAGE(host == "host" && port == "port", "expected 'host:port' found " << host << ":" << port);
}

//BOOST_AUTO_TEST_CASE( test_NodePath_perf )
//{
//	cout << "ACore:: ...test_NodePath_perf \n";
//
//	// Timing using:
//	//    StringSplitter : 6.35
//	//    Str::split     : 8.64
//	// See: Str::split -> define USE_STRINGSPLITTER
//
//	boost::timer timer;  // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
//	int n= 10000000 ;
//	std::vector<std::string> thePath;thePath.reserve(20);
//	for(int i = 0; i < n; i++) {
//	   thePath.clear();
//		NodePath::split("/this/is/a/test/string/that/will/be/used/to/check/perf/of/node/path/extraction",thePath);
//	}
//	cout << "Timing for " << n << " NodePath is  " << timer.elapsed() << endl;
//}

BOOST_AUTO_TEST_SUITE_END()
