/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#define BOOST_TEST_MODULE Test_Zombies
#include <boost/test/included/unit_test.hpp>

#include "TestFixture.hpp"

// Global test fixture.
//
// Note: Due to boost deficiency this cannot be easily accessed, so TestFixture makes use of global data.
//
BOOST_GLOBAL_FIXTURE(TestFixture);
