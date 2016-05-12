#ifndef ask_H
#define ask_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#include "uiask.h"
#include "dialog.h"
#include "str.h"
#include <string>

class ask : public dialog<ask,ask_shell_c> {
public:

	ask();

	~ask(); // Change to virtual if base class

	static bool show(str&,std::string);
private:

	ask(const ask&);
	ask& operator=(const ask&);

	bool show(const char*,str&);
};

inline void destroy(ask**) {}
#endif
