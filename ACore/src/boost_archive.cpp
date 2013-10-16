/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <stdio.h>
#include <sstream>
#include <boost/lexical_cast.hpp>

#include "Str.hpp"
#include "boost_archive.hpp"

using namespace ecf;

// return the current archive version
int boost_archive::version()
{
   std::stringstream ss;

#if defined(BINARY_ARCHIVE)
   boost::archive::binary_oarchive oa( ss );
#elif defined(PORTABLE_BINARY_ARCHIVE)
   portable_binary_oarchive oa(ss);
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
   eos::portable_oarchive oa(ss);
#else
   boost::archive::text_oarchive oa( ss );
#endif

   return oa.get_library_version();
}


// extract the boost archive version
int boost_archive::extract_version(const std::string& boost_serial_str)
{
   int version = 0;
   sscanf(boost_serial_str.c_str(),"22 serialization::archive %d",&version);
   return version;
}

bool boost_archive::replace_version(std::string& boost_serial_str, int the_new_version)
{
   int version = extract_version(boost_serial_str);
   std::string old_version = boost::lexical_cast<std::string>(version);
   std::string new_version = boost::lexical_cast<std::string>(the_new_version);
   return Str::replace(boost_serial_str,old_version,new_version);
}


