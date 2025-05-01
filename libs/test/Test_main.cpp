/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#define BOOST_TEST_MODULE Test
#include <boost/test/included/unit_test.hpp>

#include "TestFixture.hpp"

// Global test fixture.
//
// Note: Due to boost deficiency this can't be easily accessed, so TestFixture makes use of global data.
//
BOOST_TEST_GLOBAL_FIXTURE( TestFixture );
