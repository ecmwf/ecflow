#ifndef TESTUTIL_HPP_
#define TESTUTIL_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #3 $ 
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

#include <boost/noncopyable.hpp>
#include <string>
#include <map>
#include "Defs.hpp"

// This class provides a test harness for running defs file in a client server environment
// To avoid Address in use errors, we can have client/server use a different port number
// This is more important when doing instrumentation in HP-UX, as that can take a long time.
//
class TestUtil : private boost::noncopyable {
public:


	/// Returns the location of the defs file, such thats it in the test data area
	static std::string testDataLocation( const std::string& defsFile);

};
#endif /* TESTUTIL_HPP_ */
