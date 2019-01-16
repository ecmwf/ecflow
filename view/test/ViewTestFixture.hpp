#ifndef VIEWTESTFIXTURE_HPP_
#define VIEWTESTFIXTURE_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This Fixture facilitates the test of client/server on different platforms
//
//============================================================================

#include <string>
#include "TestFixture.hpp"
class Defs;

struct ViewTestFixture : public TestFixture {

	// Constructor will invoke the server, destructor will kill the server
	// Since this class is static, the constructor/destructor can not call
	// any of BOOST MACRO, since the unit test will not be there.
	// When running across platforms will will assume server is already running
   ViewTestFixture() : TestFixture("view") {}
	~ViewTestFixture() {}
};

#endif
