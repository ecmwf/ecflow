/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
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
#include <boost/foreach.hpp>
#include "WhyCmd.hpp"
#include "Defs.hpp"
#include "Node.hpp"

WhyCmd::WhyCmd(defs_ptr defs, const std::string& absNodePath)
: defs_(defs)
{
	if (!defs_.get()) {
		throw std::runtime_error("WhyCmd: The definition parameter is empty");
	}

	if (! absNodePath.empty() ) {
		node_ = defs_->findAbsNode(absNodePath);
		if ( ! node_.get() ) {
			std::string errorMsg = "WhyCmd: The node path parameter '";
			errorMsg += absNodePath;
			errorMsg += "' can not be found.";
			throw std::runtime_error(errorMsg);
		}
	}
}

std::string WhyCmd::why() const
{
	std::vector<std::string> theReasonWhy;
	if (node_.get()) {
		node_->bottom_up_why(theReasonWhy);
	}
	else {
		defs_->top_down_why(theReasonWhy);
	}

	// Don't add /n on very last item
	std::string reason;
	for(size_t i = 0; i < theReasonWhy.size(); ++i) {
 		reason += theReasonWhy[i];
		if (i != theReasonWhy.size()-1)  reason += "\n";
	}
	return reason;
}

