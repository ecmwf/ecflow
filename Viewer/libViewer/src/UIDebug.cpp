//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <iostream>

#include "UIDebug.hpp"


void UIDebug::uiAssert(char const* expr, char const* file, long line, const std::string& message)
{
	std::stringstream ss;
	ss << "ASSERT failure: " << expr << " at " << file << ":" << line << " " << message;
	std::string assert_msg = ss.str();
	std::cerr << assert_msg << "\n";
	exit(1);
}

std::string UIDebug::longToString(long num)
{
	char buf[64];
	sprintf(buf, "%ld", num);
	return std::string(buf);
}
