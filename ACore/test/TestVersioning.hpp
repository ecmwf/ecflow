// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>


/// To simulate changing of data model over time, we will
/// namespace's. The actual serialisation does not appears to
/// persist the name space

namespace version0 {
class X {
public:
   X(int h = 0) : hour_(h) {}
   bool operator==(const X& rhs) const { return hour_ == rhs.hour_; }
private:
   int hour_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int version) {
      ar & hour_;
   }
};
}

namespace version_new_data_member {
class X {
public:
   X(int h = 0, int m =0) : hour_(h),min_(m) {}
   bool operator==(const X& rhs) const { return hour_ == rhs.hour_ && min_ == rhs.min_; }
private:
   int hour_;
   int min_;
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int version) {
      // When *loading* the version pertains to loaded version in the data
      // When *saving* the version always pertains to the latest version
      ar & hour_;
      if (version > 0) ar & min_;
   }
};
}
BOOST_CLASS_VERSION(version_new_data_member::X, 1)

namespace version_change_dm_name {
class X {
public:
   X(int h = 0) : hours_(h) {}
   bool operator==(const X& rhs) const { return hours_ == rhs.hours_; }
private:
   int hours_;
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int version) {
      ar & hours_;
   }
};
}
BOOST_CLASS_VERSION(version_change_dm_name::X, 1)

namespace version_change_dm_type {
class X {
public:
   X(const std::string& h = "") : hour_(h) {}
   bool operator==(const X& rhs) const { return hour_ == rhs.hour_; }
   std::string str() const { return hour_; }
private:
   std::string hour_;
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int version) {
      // When *loading* the version pertains to loaded version in the data
      // When *saving* the version always pertains to the latest version
      if (version == 0) {
         // Change data member type: int(version0)--->string(version1)
         int the_old_hour = 0;
         ar & the_old_hour;
         hour_ = boost::lexical_cast<std::string>(the_old_hour);
      }
      else {
         ar & hour_;
      }
   }
};
}
BOOST_CLASS_VERSION(version_change_dm_type::X, 1)
