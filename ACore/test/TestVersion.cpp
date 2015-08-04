//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $
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
#include <boost/test/unit_test.hpp>
#include "Version.hpp"
#include "boost_archive.hpp"
#include "File.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
using namespace boost;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_version )
{
   std::string desc = Version::description();
   BOOST_CHECK_MESSAGE(!desc.empty(),"Expected version");
   cout << "ACore:: ...test_version:" << desc  << endl;
}

BOOST_AUTO_TEST_CASE( boost_serialisation_archive_version )
{
   cout << "ACore:: ...boost_serialisation_archive_version:  " << ecf::boost_archive::version()  << endl;
   BOOST_REQUIRE_MESSAGE(ecf::boost_archive::version() != 0,"keep boost from complaining");
}


BOOST_AUTO_TEST_CASE( test_version_against_VERSION_cmake )
{
   cout << "ACore:: ...test_version_against_VERSION_cmake" << endl;

   // Open the file VERSION.cmake
   std::string version_cmake_file = File::root_source_dir() + "/VERSION.cmake";
   std::vector<std::string> lines;
   BOOST_REQUIRE_MESSAGE(File::splitFileIntoLines(version_cmake_file,lines,true/* impore empty lines */),"Failed to open file " << version_cmake_file);
   BOOST_REQUIRE_MESSAGE(!lines.empty(),"File " << version_cmake_file << " does not contain version info ??");

   // Expecting lines like:
   //   set( ECFLOW_RELEASE  "4" )
   //   set( ECFLOW_MAJOR    "0" )
   //   set( ECFLOW_MINOR    "4" )
   //   set( ${PROJECT_NAME}_VERSION_STR  "${ECFLOW_RELEASE}.${ECFLOW_MAJOR}.${ECFLOW_MINOR}" )
   // Compare against VERSION
   std::string ecflow_release,ecflow_major,ecflow_minor;
   for(size_t i =0; i < lines.size(); ++i) {
      std::vector<std::string> tokens;
      Str::split(lines[i],tokens);

      // expecting third token to contain version data
      if (lines[i].find("set( ECFLOW_RELEASE") != std::string::npos && tokens.size() >= 3) {
         ecflow_release = tokens[2];
         Str::removeQuotes(ecflow_release);
      }
      if (lines[i].find("set( ECFLOW_MAJOR") != std::string::npos && tokens.size() >= 3) {
         ecflow_major = tokens[2];
         Str::removeQuotes(ecflow_major);
      }
      if (lines[i].find("set( ECFLOW_MINOR") != std::string::npos && tokens.size() >= 3) {
         ecflow_minor = tokens[2];
         Str::removeQuotes(ecflow_minor);
      }
   }

   std::string extracted_version = ecflow_release + "." + ecflow_major + "." + ecflow_minor;

   // The if they don't match, we have failed to regenrate and check in ecflow_version.h
   BOOST_REQUIRE_MESSAGE(Version::raw() == extracted_version,"\n  Expected " << extracted_version << " but found " << Version::raw() << ", Please regenerate file $WK/ACore/src/ecflow_version.h by calling 'sh -x $WK/cmake.sh debug'");
}

BOOST_AUTO_TEST_SUITE_END()
