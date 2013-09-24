#ifndef directory_H
#define directory_H
//=============================================================================================
// Name        : 
// Author      : 
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
//=============================================================================================


#include <stdio.h>

class directory {
public:

	directory();

	~directory(); // Change to virtual if base class

	static FILE* open(const char*,const char*);
	static const char* user();
	static const char* system();

private:

	directory(const directory&);
	directory& operator=(const directory&);

};

inline void destroy(directory**) {}
#endif
