#ifndef pref_window_H
#define pref_window_H
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


#include "uipref.h"

class pref_window : public pref_shell_c {
public:

	pref_window();

	~pref_window(); // Change to virtual if base class

	static void show();

private:

	pref_window(const pref_window&);
	pref_window& operator=(const pref_window&);

	void raise();

	static pref_window& instance();

	void closeCB(Widget,XtPointer);
	void mapCB(Widget,XtPointer);

	/* void searchCB(Widget,XtPointer); */
	/* void whatCB(Widget,XtPointer); */
	/* void whereCB(Widget,XtPointer); */
	/* void statusCB(Widget,XtPointer); */
	/* void typeCB(Widget,XtPointer); */
	/* void specialCB(Widget,XtPointer); */
};

inline void destroy(pref_window**) {}
#endif
