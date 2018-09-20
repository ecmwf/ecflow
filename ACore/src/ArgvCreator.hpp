#ifndef ARGVCREATOR_HPP_
#define ARGVCREATOR_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <vector>
#include <string>

class ArgvCreator {
public:
	// Create argc/argv from a vector of strings
	explicit ArgvCreator(const std::vector<std::string> & );

	// Destroys argv array
	~ArgvCreator();

	int argc() const     { return argc_;}
	char** argv() const  { return argv_;}

	// for debug
	std::string toString() const;

private:
	ArgvCreator(const ArgvCreator&) = delete;
	const ArgvCreator& operator=(const ArgvCreator&) = delete;

	int    argc_;
	char** argv_;
};

#endif
