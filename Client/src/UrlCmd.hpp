#ifndef URL_CMD_HPP_
#define URL_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Version     : Beta version for test use only
// Revision    : $Revision$ 
//
// Copyright 2009-2017 ECMWF.
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

class UrlCmd : private boost::noncopyable {
public:
	/// Will throw std::runtime_error if defs or node path is not correct
	UrlCmd(defs_ptr defs, const std::string& absNodePath );

	/// Will throw std::runtime_error if url can not be formed
	std::string getUrl() const;

	/// Execute the url command
	void execute() const;

private:
	defs_ptr defs_;
	Node* node_;
};

#endif
