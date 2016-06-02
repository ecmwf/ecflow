//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include "ArgvCreator.hpp"

using namespace boost;
using namespace std;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

static void doCheck(const std::vector<std::string>& theArgs)
{
	ArgvCreator argvCr(theArgs);
//	cout << argvCr.toSString() << "\n";
	BOOST_CHECK_MESSAGE(argvCr.argc() == static_cast<int>(theArgs.size()), " argc incorrect");

	char** argv = argvCr.argv();
	for(int i=0; i < argvCr.argc(); i++) {
		BOOST_CHECK_MESSAGE(string(argv[i]) == theArgs[i],"Mismatch in args expected " << theArgs[i] << " but found " << argv[i]);
//		cout << i << ": " << argv[i] << "\n";
	}
}

BOOST_AUTO_TEST_CASE( test_ArgvCreator )
{
	cout << "ACore:: ...test_ArgvCreator" << flush ;

	// O args
	std::vector<std::string> theArgs;
	doCheck(theArgs);

	// 1 args
	theArgs.push_back("arg1");
	doCheck(theArgs);

	// 2 args
	theArgs.clear();
	theArgs.push_back("arg1");
	theArgs.push_back("arg2");
	doCheck(theArgs);

	// 10 args
	theArgs.clear();
	for(int i = 0; i < 10; i++) {
		string theArg("arg");
		theArg += lexical_cast<string>(i);
		theArgs.push_back(theArg );
	}
	doCheck(theArgs);
}

BOOST_AUTO_TEST_SUITE_END()
