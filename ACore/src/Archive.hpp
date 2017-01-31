#ifndef ARCHIVE_HPP_
#define ARCHIVE_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #11 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>

namespace ecf {

// This class allows multiple archives to be used
// Since it will allow Archive::Type to be passed as a parameter, when multiple archives exist
class Archive : private boost::noncopyable {
public:

   enum Type {
      TEXT = 0,
      PORTABLE_BINARY = 1,
      BINARY = 2,
      EOS_PORTABLE_BINARY = 3
   };

   static Type default_archive() {

#if defined(BINARY_ARCHIVE)
      return BINARY;
#endif

#if defined(PORTABLE_BINARY_ARCHIVE)
      return PORTABLE_BINARY;
#endif

#if defined(EOS_PORTABLE_BINARY_ARCHIVE)
      return EOS_PORTABLE_BINARY;
#endif

      return TEXT;
   }

private:
   Archive();
};

}
#endif
