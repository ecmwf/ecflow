//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $
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
#include <fstream>

#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Log.hpp"

using namespace std;
using namespace ecf;
using namespace boost;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

static std::string dump(const std::vector<std::string>& vec)
{
   std::stringstream ss;
   std::copy (vec.begin(), vec.end(), std::ostream_iterator<std::string> (ss, "\n"));
   return ss.str();
}

BOOST_AUTO_TEST_CASE( test_defs_history_parser )
{
   cout << "ANode:: ...test_defs_history_parser\n";

   {
      string str1("MSG:[12:03:55 21.8.2013] --shutdown=yes :map");
      DefsHistoryParser parser;
      parser.parse(str1);

      std::vector<std::string> expected_messages;
      expected_messages.emplace_back("MSG:[12:03:55 21.8.2013] --shutdown=yes :map");

      BOOST_CHECK_MESSAGE(parser.parsed_messages() == expected_messages,"Expected:\n" <<  dump(expected_messages) << "but found:\n" << dump(parser.parsed_messages()));
   }
   {
      string str1("MSG:[12:03:55 21.8.2013] --shutdown=yes :mapMSG:[12:34:08 21.8.2013] --restart :map");
      DefsHistoryParser parser;
      parser.parse(str1);

      std::vector<std::string> expected_messages;
      expected_messages.emplace_back("MSG:[12:03:55 21.8.2013] --shutdown=yes :map");
      expected_messages.emplace_back("MSG:[12:34:08 21.8.2013] --restart :map");
      BOOST_CHECK_MESSAGE(parser.parsed_messages() == expected_messages,"Expected:\n" <<  dump(expected_messages) << "but found:\n" << dump(parser.parsed_messages()));
    }
   {
      string str1("MSG:[12:03:55 21.8.2013] --shutdown=yes :mapMSG:[12:34:08 21.8.2013] --restart :mapMSG:[12:47:22 21.8.2013] --alter add variable SMSNODE 0 /  :mapMSG:[13:38:45 21.8.2013] --alter add variable SMSTRYNO 0 /  :mapMSG:[13:44:09 21.8.2013] --alter add variable SMSHOME /vol/emos/output /  :mapMSG:[15:36:14 21.8.2013] --shutdown=yes :mapMSG:[16:04:26 21.8.2013] --alter add variable SMSNODE 0 /  :map");
      DefsHistoryParser parser;
      parser.parse(str1);

      std::vector<std::string> expected_messages;
      expected_messages.emplace_back("MSG:[12:03:55 21.8.2013] --shutdown=yes :map");
      expected_messages.emplace_back("MSG:[12:34:08 21.8.2013] --restart :map");
      expected_messages.emplace_back("MSG:[12:47:22 21.8.2013] --alter add variable SMSNODE 0 /  :map");
      expected_messages.emplace_back("MSG:[13:38:45 21.8.2013] --alter add variable SMSTRYNO 0 /  :map");
      expected_messages.emplace_back("MSG:[13:44:09 21.8.2013] --alter add variable SMSHOME /vol/emos/output /  :map");
      expected_messages.emplace_back("MSG:[15:36:14 21.8.2013] --shutdown=yes :map");
      expected_messages.emplace_back("MSG:[16:04:26 21.8.2013] --alter add variable SMSNODE 0 /  :map");
      BOOST_CHECK_MESSAGE(parser.parsed_messages() == expected_messages,"Expected:\n" <<  dump(expected_messages) << "but found:\n" << dump(parser.parsed_messages()));
    }
   {
      string str1("MSG:[12:03:55 21.8.2013] --shutdown=yes :mapLOG:[12:34:08 21.8.2013] --restart :mapERR:[12:47:22 21.8.2013] --alter add variable SMSNODE 0 /  :mapWAR:[13:38:45 21.8.2013] --alter add variable SMSTRYNO 0 /  :mapDBG:[13:44:09 21.8.2013] --alter add variable SMSHOME /vol/emos/output /  :mapOTH:[15:36:14 21.8.2013] --shutdown=yes :mapOTH:[16:04:26 21.8.2013] --alter add variable SMSNODE 0 /  :map");
      DefsHistoryParser parser;
      parser.parse(str1);

      std::vector<std::string> expected_messages;
      expected_messages.emplace_back("MSG:[12:03:55 21.8.2013] --shutdown=yes :map");
      expected_messages.emplace_back("LOG:[12:34:08 21.8.2013] --restart :map");
      expected_messages.emplace_back("ERR:[12:47:22 21.8.2013] --alter add variable SMSNODE 0 /  :map");
      expected_messages.emplace_back("WAR:[13:38:45 21.8.2013] --alter add variable SMSTRYNO 0 /  :map");
      expected_messages.emplace_back("DBG:[13:44:09 21.8.2013] --alter add variable SMSHOME /vol/emos/output /  :map");
      expected_messages.emplace_back("OTH:[15:36:14 21.8.2013] --shutdown=yes :map");
      expected_messages.emplace_back("OTH:[16:04:26 21.8.2013] --alter add variable SMSNODE 0 /  :map");
      BOOST_CHECK_MESSAGE(parser.parsed_messages() == expected_messages,"Expected:\n" <<  dump(expected_messages) << "but found:\n" << dump(parser.parsed_messages()));
   }
}

BOOST_AUTO_TEST_SUITE_END()
