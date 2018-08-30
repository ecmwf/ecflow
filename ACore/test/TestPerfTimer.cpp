//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description
//============================================================================
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include "perf_timer.hpp"

using namespace boost;
using namespace std;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )


static void func(int const count = 100000)
{
   //std::cout << " func : count " << count << "\n";
   for (int i = 0; i < count; i++) {
      std::string s = std::string("fred");
      s += boost::lexical_cast<std::string>(i);
   }
}

BOOST_AUTO_TEST_CASE( test_perf_with_chrono )
{
   cout << "ACore:: ...test_perf_with_chrono\n" ;

   // using namespace std::chrono_literals;

   auto t = perf_timer<>::duration(func,100000);

   std::cout << "   " << std::chrono::duration<double,std::micro>(t).count() << " micro" << std::endl;
   std::cout << "   " << std::chrono::duration<double,std::milli>(t).count() << " milli" << std::endl;
   std::cout << "   " << std::chrono::duration<double,std::nano>(t).count()  << " nano" << std::endl;

   // This does not work, ie calling func with no arguments ?? with our hacked invoke, wait till c++ 17
   //auto t0= perf_timer<std::chrono::nanoseconds>::duration(func);


   auto t1 = perf_timer<std::chrono::nanoseconds>::duration(func,10);
   auto t2 = perf_timer<std::chrono::microseconds>::duration(func,100);
   auto t3 = perf_timer<std::chrono::milliseconds>::duration(func,100000);
   std::cout << "   " << std::chrono::duration<double,std::milli>(t1 + t2 + t3).count() << " milli" << std::endl;
   BOOST_CHECK_MESSAGE(true,"dummy to keep unit test happy");
}

BOOST_AUTO_TEST_CASE( test_chrono_timer )
{
   cout << "ACore:: ... test_chrono_timer\n" ;

   // using namespace std::chrono_literals;
   {
      Timer<std::chrono::milliseconds> timer;
      func();
      timer.elapsed("   func with default args , milliseconds");
      std::cout << "    " << std::chrono::duration<double,std::micro>(timer.elapsed()).count() << " micro" << std::endl;
      std::cout << "    " << std::chrono::duration<double,std::milli>(timer.elapsed()).count() << " milli" << std::endl;
      std::cout << "    " << std::chrono::duration<double,std::nano>(timer.elapsed()).count()  << " nano" << std::endl;
   }
   {
      Timer<std::chrono::microseconds> timer;
      func();
      timer.elapsed("   func with default args , microseconds");
   }
   {
      Timer<std::chrono::nanoseconds> timer;
      func();
      timer.elapsed("   func with default args , nanoseconds");
   }
   {
      Timer<std::chrono::seconds> timer;
      func();
      timer.elapsed("   func with default args , seconds");
   }
   BOOST_CHECK_MESSAGE(true,"dummy to keep unit test happy");
}

BOOST_AUTO_TEST_SUITE_END()
