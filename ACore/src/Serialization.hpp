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

namespace ecf {

template< typename T >
void save(const std::string& fileName, const T& t)
{
   std::ofstream os(fileName);
   cereal::JSONOutputArchive oarchive(os); // Create an output archive
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
   cereal::JSONOutputArchive oarchive(archive_stream); // Create an output archive
   oarchive(cereal::make_nvp("def",t) );               // Write the data to the archive
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
