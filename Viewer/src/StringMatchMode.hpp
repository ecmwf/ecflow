//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_STRINGMATCHMODE_HPP_
#define VIEWER_SRC_STRINGMATCHMODE_HPP_

#include <string>
#include <map>

class StringMatchMode
{
public:
	enum Mode {ContainsMatch=0,WildcardMatch=1,RegexpMatch=2};

	StringMatchMode();
	StringMatchMode(Mode m);
	StringMatchMode(const StringMatchMode& r) {mode_=r.mode_;}

	Mode mode() const {return mode_;}
	void setMode(Mode m) {mode_=m;}
	const std::string& matchOperator() const;
	int toInt() const {return static_cast<int>(mode_);}

	static Mode operToMode(const std::string&);

private:
	void init();

	Mode mode_;
	static std::map<Mode,std::string> matchOper_;
};


#endif /* VIEWER_SRC_STRINGMATCHMODE_HPP_ */
