#ifndef PLUGCMD_CONTEXT_HPP_
#define PLUGCMD_CONTEXT_HPP_
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
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>

namespace  ecf {

class PlugCmdContext : private boost::noncopyable {
public:
   PlugCmdContext();
   ~PlugCmdContext();

   static bool in_plug() { return in_plug_; }

private:
   static bool in_plug_;
};
}

#endif
