#ifndef BOOST_ARCHIVE_HPP_
#define BOOST_ARCHIVE_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class has come about due to a bug with boost archive
//               includes. i.e if the same set of includes are not defined
//               in different compilations units. Then _NO_ errors/warnings are
//               issued instead we get a crash at run time when serialising
//               via base pointer.
//
//               To get round this code will use this include to collate the
//               archives used in a single place.
//============================================================================

#if defined(TEXT_ARCHIVE) || !defined(BINARY_ARCHIVE) && !defined(PORTABLE_BINARY_ARCHIVE) && !defined(EOS_PORTABLE_BINARY_ARCHIVE)
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#endif

#if defined(BINARY_ARCHIVE)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#endif

#if defined(PORTABLE_BINARY_ARCHIVE)
#include "portable_binary_oarchive.hpp"
#include "portable_binary_iarchive.hpp"
#endif

#if defined(EOS_PORTABLE_BINARY_ARCHIVE)
#include "portable_oarchive.hpp"
#include "portable_iarchive.hpp"
#endif

#include <boost/noncopyable.hpp>

namespace ecf {

/// Utility class for boost archive version
///
/// Boost archive version is specified in: $BOOST_ROOT/libs/serialization/src/basic_archive.cpp
///
/// boost 1.47 serialisation library archive version = 9
/// boost 1.53 serialisation library archive version = 10
/// boost 1.56 serialisation library archive version = 11 // however no change in library ?
/// boost 1.57 serialisation library archive version = 11
///
/// boost supports old -> new only. In our case typically new_client needs to talk to old server
/// Hence if new client archive version is newer we need to set to archive version used by server.
/// *providing* there are compatible.
///
/// To enable this, user can export variable ECF_ALLOW_NEW_CLIENT_OLD_SERVER:
///
/// We expect following syntax:
///    option 1/ export ECF_ALLOW_NEW_CLIENT_OLD_SERVER=<int>
///              This for use ecflow_client command line
///
///    option 2/ export ECF_ALLOW_NEW_CLIENT_OLD_SERVER=<host>:<port>:<int>,<host>:<port>:<int>,<host>:<port>:<int>
///              This for use with ui/viewer where we can have multiple clients, each could
///              connect to different server version and hence archive.
///
/// export ECF_ALLOW_NEW_CLIENT_OLD_SERVER=10, the number used, must be the archive version
/// that the boost server was built with.
///
class boost_archive : private boost::noncopyable {
public:

   // return the current archive version
   static int version();

   // extract the boost archive version, assumes text archive *****
   static int extract_version(const std::string&);

   // replace archive version, return true if replace worked.
   static bool replace_version(std::string&, int new_version);

   // avoid hard coding
   static int version_1_47() { return 9;}

private:
   boost_archive();
};

}
#endif
