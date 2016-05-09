#ifndef search_H
#define search_H

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

#ifdef NO_BOOL
#include "bool.h"
#endif

#include "uisearch.h"
#include "node_lister.h"
#include "array.h"

class flags;

class search : public search_shell_c, public node_lister {
public:

	search();

	~search(); // Change to virtual if base class

	static void show();

private:

	search(const search&);
	search& operator=(const search&);

	char *name_;
	array<flags*> status_flags_;
	array<flags*> special_flags_;
	array<flags*> type_flags_;
	int  timed_since_, timed_from_, subs_, rege_, icas_, glob_ ;

// -- Methods

	void raise();
	void scan(Widget,array<flags*>&);


// -- Overridden methods

	void next(node&);
	bool check(node&,array<flags*>&);

// -- Class members

	static search& instance();

// -- Class methods

	void closeCB(Widget,XtPointer);
	void searchCB(Widget,XtPointer);
	void mapCB(Widget,XtPointer);

	void whatCB(Widget,XtPointer);
	void whereCB(Widget,XtPointer);
	void statusCB(Widget,XtPointer);
	void typeCB(Widget,XtPointer);
	void specialCB(Widget,XtPointer);
	void timedCB(Widget,XtPointer);
	void miscCB(Widget,XtPointer);
	void radioCB(Widget,XtPointer);
};

inline void destroy(search**) {}

#endif
