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

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include <cereal/archives/json.hpp>

using namespace boost;
using namespace std;
namespace fs = boost::filesystem;


class MyClass {
public:
   MyClass() : x(2),y(2),z(2) {}

private:
   // This method lets cereal know which data members to serialize
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & archive, std::uint32_t const version) {
      archive( CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z) );
   }
   int x, y, z;
};
CEREAL_CLASS_VERSION(MyClass , 1);

class MyTop : public MyClass {
public:
   MyTop() : x_(1),y_(1),z_(1) {}

private:
   // This method lets cereal know which data members to serialize
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & archive, std::uint32_t const version) {
      archive( cereal::base_class<MyClass>( this ),
               CEREAL_NVP(x_), CEREAL_NVP(y_), CEREAL_NVP(z_) );
   }
   int x_, y_, z_;
};
CEREAL_CLASS_VERSION(MyTop  , 1);


BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_cereal_json )
{
   cout << "ACore:: ...test_cereal_json \n" ;
   std::string path = "test_cereal_json";
   {
      std::ofstream os(path);
      cereal::JSONOutputArchive oarchive(os); // Create an output archive

      MyTop  m1, m2, m3;
      oarchive(CEREAL_NVP(m1), m2, m3); // Write the data to the archive
   } // archive goes out of scope, ensuring all contents are flushed

   {
      BOOST_CHECK_MESSAGE(fs::exists(path)," Expected file to exist");

      std::ifstream is(path);
      cereal::JSONInputArchive iarchive(is); // Create an input archive

      MyTop   m1, m2, m3;
      iarchive(m1, m2, m3); // Read the data from the archive

      fs::remove(path); // Remove the file. Comment out for debugging
   }
}

BOOST_AUTO_TEST_SUITE_END()
