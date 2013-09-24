#ifndef PORT_HPP_
#define PORT_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
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

#include <string>

namespace ecf {

// If two different process/servers both try to use the same port number, you
// get an "Address in use" error, even if one the process is dead. This is
// because the kernel does not immediately release the resource and there is
// time out period. To get round this we will start the new server/client
// and use a different port number.

class SCPort {
public:

	/// make sure we have a unique port, each time next() is called;
	static std::string next();

private:
	SCPort();
	~SCPort();

	static int thePort_;
};
}

#endif
