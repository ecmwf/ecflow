 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
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
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

using namespace std;
namespace po = boost::program_options;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_program_options_implicit_value )
{
   cout << "Base:: ...test_program_options_implicit_value\n";

   // Declare the supported options.
   po::options_description desc("Allowed options");
   desc.add_options()
       ("help", "produce help message")
       ("arg1", po::value<string>()->implicit_value( string("") ), "optional arg1 description") ;

   {
      char* argv[] = {
            const_cast<char*>("test_program_options_implicit_value"),
            const_cast<char*>("--help"),
            const_cast<char*>("--arg1")
      };

      po::variables_map vm;
      po::store(po::parse_command_line(3, argv, desc), vm);  // populate variable map
      po::notify(vm);                                        // raise any errors

      BOOST_CHECK_MESSAGE(vm.count("help"), "Expected help");
      BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");
      BOOST_CHECK_MESSAGE(vm["arg1"].as<string>() == "", "Expected arg1 to be empty");
   }
//   {
//      // ******* This test fails on boost 1.59, can't cope --arg1 10, only --arg1=10 *******
//      char* argv[] = {
//             const_cast<char*>("test_program_options_implicit_value"),
//             const_cast<char*>("--arg1"),
//             const_cast<char*>("10")
//       };
//
//       po::variables_map vm;
//       po::store(po::parse_command_line(3, argv, desc), vm);
//       po::notify(vm);
//
//       BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");
//       BOOST_CHECK_MESSAGE(vm["arg1"].as<string>() == "10", "Expected arg1 with value of 10 but found '" << vm["arg1"].as<string>() << "'");
//   }
   {
      char* argv[] = {
             const_cast<char*>("test_program_options_implicit_value"),
             const_cast<char*>("--arg1=11")
       };

       po::variables_map vm;
       po::store(po::parse_command_line(2, argv, desc), vm);
       po::notify(vm);

       BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");
       BOOST_CHECK_MESSAGE(vm["arg1"].as<string>() == "11", "Expected arg1 with value of 11 but found " << vm["arg1"].as<string>());
   }
}


BOOST_AUTO_TEST_CASE( test_program_options_multitoken )
{
   cout << "Base:: ...test_program_options_multitoken\n";

   // Declare the supported options.
   po::options_description desc("Allowed options");
   desc.add_options()
       ("help", "produce help message")
       ("arg1", po::value< vector<string> >()->multitoken(), "arg1 description") ;

   char* argv[] = {
         const_cast<char*>("test_program_options_multitoken"),
         const_cast<char*>("--help"),
         const_cast<char*>("--arg1"),
         const_cast<char*>("a"),
         const_cast<char*>("b")
   };

   po::variables_map vm;
   po::store(po::parse_command_line(5, argv, desc), vm);
   po::notify(vm);

   BOOST_CHECK_MESSAGE(vm.count("help"), "Expected help");
   BOOST_CHECK_MESSAGE(vm.count("arg1"), "Expected arg1");

   std::vector<string> expected; expected.push_back("a"); expected.push_back("b");
   BOOST_CHECK_MESSAGE(vm["arg1"].as< vector<string> >() == expected, "multi-token not as expected");
}

BOOST_AUTO_TEST_SUITE_END()

