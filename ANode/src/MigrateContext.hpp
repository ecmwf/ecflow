#ifndef MIGRATE_CONTEXT_HPP_
#define MIGRATE_CONTEXT_HPP_
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
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>

namespace  ecf {

class MigrateContext : private boost::noncopyable {
   // Used when --migrate called
   // Allow us to dump children
public:
   MigrateContext();
   ~MigrateContext();

   static bool in_migrate() { return in_migrate_; }

private:
   static bool in_migrate_;
};
}

#endif
