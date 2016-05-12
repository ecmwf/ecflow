#ifndef scripting_H
#define scripting_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#ifndef extent_H
#include "extent.h"
#endif

class scripting : public extent<scripting> {
public:

	scripting(const char*);

	~scripting(); // Change to virtual if base class

	virtual int execute(int,char**) = 0;

	static void init();
	static void run(const char*);
	static void execute(const char*);

	static scripting* find(const char* name);

	static int dispatch(int,char**);

private:

	scripting(const scripting&);
	scripting& operator=(const scripting&);
	
	const char* name_;
};

inline void destroy(scripting**) {}
#endif
