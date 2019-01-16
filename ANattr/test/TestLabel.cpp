/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>
#include <iostream>
#include <fstream>
#include <boost/test/unit_test.hpp>

#include "NodeAttr.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_label_parsing )
{
   cout << "ANattr:: ...test_label_parsing\n";
   {
      std::string line = "label name \"value\"";
      std::vector<string> linetokens;
      Str::split(line,linetokens);

      Label label;
      label.parse(line,linetokens, false);

      Label expected("name","value");
      BOOST_CHECK_MESSAGE(label == expected,"Expected " << expected.toString() << " but found " << label.toString());
      BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),"Expected " << expected.dump() << " but found " << label.dump());
   }
   {
      std::string line = "label name \"value\\nvalue\"";
      std::vector<string> linetokens;
      Str::split(line,linetokens);

      Label label;
      label.parse(line,linetokens, false);

      Label expected("name","value\nvalue");
      BOOST_CHECK_MESSAGE(label == expected,"Expected " << expected.toString() << " but found " << label.toString());
      BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),"Expected " << expected.dump() << " but found " << label.dump());
   }
   {
      std::string line = "label name \"value that is multiple token !!!! 23445 !^ & * ( )\"";
      std::vector<string> linetokens;
      Str::split(line,linetokens);

      Label label;
      label.parse(line,linetokens, false);

      Label expected("name","value that is multiple token !!!! 23445 !^ & * ( )");
      BOOST_CHECK_MESSAGE(label == expected,"Expected " << expected.toString() << " but found " << label.toString());
      BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),"Expected " << expected.dump() << " but found " << label.dump());
   }
   {
      std::string line = "label name \"value\\n that\\n is\\n multiple\\n token\\n and\\n new\\n \\nlines\"";
      std::vector<string> linetokens;
      Str::split(line,linetokens);

      Label label;
      label.parse(line,linetokens, false);

      Label expected("name","value\n that\n is\n multiple\n token\n and\n new\n \nlines");
      BOOST_CHECK_MESSAGE(label == expected,"Expected " << expected.toString() << " but found " << label.toString());
      BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),"Expected " << expected.dump() << " but found " << label.dump());
   }
}

BOOST_AUTO_TEST_SUITE_END()

