#ifndef str_H
#define str_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


#include <string>
#ifdef NO_BOOL
#include "bool.h"
#endif

#include "counted.h"

struct str_imp : public counted {
	char* str_;
	str_imp(const char*);
	~str_imp();
};

class str {
public:
	str();
	str(const char*);
	str(const str&);
	str(const std::string&);

	~str(); // Change to virtual if base class

	str& operator=(const str&);

	bool operator==(const str&) const;
	bool operator!=(const str&) const;

	str& operator+=(const str&);

	const char* c_str() const { return imp_->str_; }

private:

	str_imp* imp_;
};

str operator+(const str&,const str&);
str operator+(const char*,const str&);
str operator+(const str&,const char*);

#endif
