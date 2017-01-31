#ifndef SUITE_CHANGED_HPP_
#define SUITE_CHANGED_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
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
#include "NodeFwd.hpp"

namespace ecf {

// Determine if suite was changed or modified if so, update suite change no
// This mechanism was used because, when changing some attributes, we can not
// immediately access the parent suites, to update the change numbers.
//
// When given a choice between where to add SuiteChanged, i.e in Node Tree or Commands
// Generally favour commands, as it will require less maintenance over time.
//
// This mechanism was added specifically to support changes over client handles
// i.e suites are added to handles, hence we need a way to determine which
// suites (and hence handle) changed, and hence minimise the need for updates.

class SuiteChanged  : private boost::noncopyable {
public:
	SuiteChanged(suite_ptr s);
	~SuiteChanged();
private:
	weak_suite_ptr suite_;
	unsigned int state_change_no_;
	unsigned int modify_change_no_;
};

class SuiteChanged0  : private boost::noncopyable {
public:
	SuiteChanged0(node_ptr s);
	~SuiteChanged0();
private:
	weak_node_ptr node_;
   Suite* suite_; // if node is removed suite pointer is not accessible, hence store first
	unsigned int state_change_no_;
	unsigned int modify_change_no_;
};


class SuiteChanged1  : private boost::noncopyable {
public:
	SuiteChanged1(Suite* s);
	~SuiteChanged1();
private:
	Suite* suite_;
	unsigned int state_change_no_;
	unsigned int modify_change_no_;
};

}
#endif
