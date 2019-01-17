#ifndef window_H
#define window_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <Xm/Xm.h>

#include "extent.h"

class window : public extent<window> {
public:

	window();

	~window(); // Change to virtual if base class

	void raise();
	virtual Widget shell() = 0;

	void set_menu(const char*);

private:

	window(const window&);
	window& operator=(const window&);

	Widget menu_;
};

inline void destroy(window**) {}

#endif
