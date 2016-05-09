#ifndef str_H
#include "str.h"
#endif
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

#include <string.h>
#include <strings.h>
#include <stdio.h>

str_imp::str_imp(const char* p):
	str_ (new char[strlen(p) + 1])
{
	//if(!str_) { printf("Out of memory"); exit(1); }	
	strcpy(str_,p);
}

str_imp::~str_imp()
{
	delete[] str_;
}


str::str()
{
	static str empty("");
	imp_ = empty.imp_;
	imp_->attach();	
}

str::~str()
{
	imp_->detach();
}

str::str(const char* n):
	imp_(new str_imp(n))
{
	imp_->attach(); 
}

str::str(const str& other):
	imp_(other.imp_)
{
	imp_->attach(); 
}

str::str(const std::string& other):
  imp_(new str_imp(other.c_str()))
{
	imp_->attach(); 
}

str& str::operator=(const str& other)
{
	other.imp_->attach();
	imp_->detach();
	imp_ = other.imp_;
	return *this;
}


bool str::operator==(const str& other) const
{
	return strcmp(c_str(),other.c_str()) == 0;
}

bool str::operator!=(const str& other) const
{
	return strcmp(c_str(),other.c_str()) != 0;
}

str operator+(const str& a,const str& b)
{
	char* p = new char[strlen(a.c_str()) + strlen(b.c_str()) + 1 ];
	strcpy(p,a.c_str());
	strcat(p,b.c_str());
	str s(p);
	delete[] p;
	return s;
}

str& str::operator+=(const str& s)
{
	*this = *this + s;
	return *this;
}
