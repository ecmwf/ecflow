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
#include <sstream>
#include <fstream>
#include <iostream>
#include <memory>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "SerializationTest.hpp"

using namespace ecf;
using namespace boost;
using namespace std;
namespace fs = boost::filesystem;


class Base {
public:
   enum State { UNKNOWN =0, COMPLETE=1,  QUEUED=2, ABORTED=3, SUBMITTED=4, ACTIVE=5, SUSPENDED=6};
   Base() : x(2),y(2),state_(State::SUSPENDED),test_(false) {}
   Base(bool test) : x(2),y(2),state_(State::SUSPENDED),test_(test) {}
   bool operator==(const Base & rhs) const { return x == rhs.x && y == rhs.y && state_ == rhs.state_ && test_ == rhs.test_;}
   void print(std::ostream &os) const {
      os << "Base: x("<< x << ") y(" << y << ") state(" << state_ << ") test(" << test_ << ")";
   }
private:
   // This method lets cereal know which data members to serialize
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version) {
      ar( CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(state_) );
      CEREAL_OPTIONAL_NVP(ar, test_, [this](){return test_; }); // conditionally save
   }
   int x, y;
   State state_;
   bool test_;
};

std::ostream& operator<<(std::ostream &os, Base const &m) {
   m.print(os);
   return os;
}

// =================================================================================

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_cereal_optional )
{
   cout << "ACore:: ...test_cereal_optional\n" ;
   Base  original;
   Base  original1(true);
   std::string path = "test_cereal_optional";
   {
      std::ofstream os(path);
      cereal::JSONOutputArchive oarchive(os); // Create an output archive
      oarchive(
               cereal::make_nvp("Base",original),
               cereal::make_nvp("Base",original1)
      ); // Write the data to the archive
   } // archive goes out of scope, ensuring all contents are flushed

   {
      BOOST_CHECK_MESSAGE(fs::exists(path)," Expected file to exist");

      std::ifstream is(path);
      cereal::JSONInputArchive iarchive(is); // Create an input archive

      Base restored ;
      Base restored1 ;
      iarchive(restored,restored1); // Read the data from the archive
      BOOST_CHECK_MESSAGE(restored == original,"restored(" << restored  << ") != original(" << original << ")");
      BOOST_CHECK_MESSAGE(restored1 == original1,"restored1(" << restored1  << ") != original1(" << original1 << ")");
      //fs::remove(path); // Remove the file. Comment out for debugging
   }
}

BOOST_AUTO_TEST_SUITE_END()
