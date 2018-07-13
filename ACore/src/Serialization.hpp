#ifndef SERIALIZATION_HPP_
#define SERIALIZATION_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Simple class that defines the Archive types used for
//               Serialisation
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>
#include <iostream>
#include <fstream>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/utility.hpp>

#include "cereal_optional_nvp.hpp"

namespace ecf {

template< typename T >
void save(const std::string& fileName, const T& t)
{
   std::ofstream os(fileName);
#ifdef DEBUG
   cereal::JSONOutputArchive oarchive(os); // Use default Indent can be very slow
#else
   cereal::JSONOutputArchive oarchive(os, cereal::JSONOutputArchive::Options::NoIndent()); // Create an output archive
#endif
   oarchive(cereal::make_nvp(typeid(t).name(),t) ); // Write the data to the archive
}

template< typename T >
void restore(const std::string& fileName, T& restored)
{
   std::ifstream is(fileName);
   cereal::JSONInputArchive iarchive(is); // Create an input archive
   iarchive(restored);                    // Read the data from the archive
}


template< typename T >
void save_as_string(std::string& outbound_data, const T& t)
{
   std::ostringstream archive_stream;
   {
#ifdef DEBUG
      cereal::JSONOutputArchive oarchive(archive_stream);  // Use default Indent can be very slow
#else
      cereal::JSONOutputArchive oarchive(archive_stream,cereal::JSONOutputArchive::Options::NoIndent()); // Create an output archive
#endif
      // when archive goes out of scope it is guaranteed to have flushed its
      // contents to its stream.
      oarchive(cereal::make_nvp(typeid(t).name(),t) );    // Write the data to the archive
   }
   outbound_data = archive_stream.str();
}

template< typename T >
void restore_from_string(const std::string& archive_data, T& restored)
{
   std::istringstream archive_stream(archive_data);
   cereal::JSONInputArchive iarchive(archive_stream); // Create an input archive
   iarchive(restored);                                // Read the data from the archive
}

}

#endif
