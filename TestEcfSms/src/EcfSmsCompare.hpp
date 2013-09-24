#ifndef ECF_SMS_COMPARE_HPP_
#define ECF_SMS_COMPARE_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include "NodeFwd.hpp"

class EcfSmsCompare : private boost::noncopyable {
public:
	EcfSmsCompare(const std::string& comparisonFile) : comparisonFile_(comparisonFile) {}

 	/// Will open the comparison file and and ensure that the paths, states, event, meter, repeats
	/// can be found with the same value as the input defs.
	/// Additionally we cross-check family/nodes in the defs file exist in the comparison file
	bool run(const defs_ptr&, std::string& errorMsg);

private:
 	void addLineToErrorMsg(size_t i,const std::string& line, std::string& errorMsg ) const;

	 std::string comparisonFile_;
};

#endif
