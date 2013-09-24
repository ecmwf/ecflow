#ifndef SERIALIZATION_HPP_
#define SERIALIZATION_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2012 ECMWF. 
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

#include "boost_archive.hpp"
#include "Archive.hpp"

namespace ecf {

/// These function do *NOT* trap  boost::archive::archive_exception since we want it
/// to propagate up.

template< typename T >
void save(const std::string& fileName, const T& ts, ecf::Archive::Type at = ecf::Archive::default_archive())
{
   // Argument added if in future we want to allow multiple archives, so we can choose
#if defined(BINARY_ARCHIVE)
	std::ofstream ofs( fileName.c_str(), std::ios::binary );
	boost::archive::binary_oarchive oa( ofs );
	oa << ts;
#elif defined(PORTABLE_BINARY_ARCHIVE)
	std::ofstream ofs( fileName.c_str(), std::ios::binary );
	portable_binary_oarchive oa(ofs);
	oa << ts;
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
   std::ofstream ofs( fileName.c_str(), std::ios::binary );
   eos::portable_oarchive oa(ofs);
   oa << ts;
#else
   std::ofstream ofs( fileName.c_str() );
   boost::archive::text_oarchive oa( ofs );
	oa << ts;
#endif
}

template< typename T >
void save_as_string(std::string& outbound_data, const T& t)
{
   std::ostringstream archive_stream;

#if defined(BINARY_ARCHIVE)
   boost::archive::binary_oarchive archive( archive_stream );
   archive << t;
   outbound_data = archive_stream.str();
   //             std::cout << "async_write BINARY " << outbound_data_ << "\n";
#elif defined(PORTABLE_BINARY_ARCHIVE)
   portable_binary_oarchive archive( archive_stream );
   archive << t;
   outbound_data = archive_stream.str();
   //             std::cout << "async_write PORTABLE_BINARY " << outbound_data_ << "\n";
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
   eos::portable_oarchive archive( archive_stream );
   archive << t;
   outbound_data = archive_stream.str();
   //             std::cout << "async_write EOS_PORTABLE_BINARY " << outbound_data_ << "\n";
#else
   boost::archive::text_oarchive archive( archive_stream );
   archive << t;
   outbound_data = archive_stream.str();
#endif
}

template< typename T >
void restore(const std::string& fileName, T& restored, ecf::Archive::Type at = ecf::Archive::default_archive())
{
   // Argument added if in future we want to allow multiple archives, so we can choose

#if defined(BINARY_ARCHIVE)
	std::ifstream ifs( fileName.c_str(), std::ios::binary );
	boost::archive::binary_iarchive ia( ifs );
	ia >> restored;
#elif defined(PORTABLE_BINARY_ARCHIVE)
	std::ifstream ifs( fileName.c_str(), std::ios::binary );
	portable_binary_iarchive ia( ifs );
	ia >> restored;
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
   std::ifstream ifs( fileName.c_str(), std::ios::binary );
   eos::portable_iarchive ia( ifs );
   ia >> restored;
#else
	std::ifstream ifs( fileName.c_str() );
	boost::archive::text_iarchive ia( ifs );
	ia >> restored;
#endif
}

}
#endif
