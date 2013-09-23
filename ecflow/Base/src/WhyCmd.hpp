#ifndef WHY_CMD_HPP_
#define WHY_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Client side command only.
//               Placed in this category, since the server does not need to link
//               with it.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <string>
#include <boost/noncopyable.hpp>
#include "NodeFwd.hpp"

class WhyCmd : private boost::noncopyable {
public:
	WhyCmd(defs_ptr defs, const std::string& absNodePath);

	/// Why the node is not running
	/// Return a '/n' separated string which lists the reasons why
	/// the provided node is not active.
	std::string why() const;

private:
	defs_ptr defs_;
	node_ptr node_;
};

#endif
