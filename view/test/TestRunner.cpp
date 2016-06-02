#define BOOST_TEST_MODULE TestView
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
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
#include "ViewTestFixture.hpp"
#include <boost/test/unit_test.hpp>

// Global test fixture. Dues to boost deficiency this can't be accessed. hence
// TestFixture makes use of global data.
BOOST_GLOBAL_FIXTURE( ViewTestFixture );
