#ifndef searchable_H
#define searchable_H
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


#ifdef NO_BOOL
#include "bool.h"
#endif

#ifndef ecflowview_H
#include "ecflowview.h"
#endif

#ifndef extent_H
#include "extent.h"
#endif

class node_lister;

class searchable : public extent<searchable> {
public:
	searchable();

	virtual ~searchable(); // Change to virtual if base class

	void active(Boolean);

	virtual const char* name() const = 0;
	virtual void search(node_lister&) = 0;

	static void look_for(node_lister&,bool);
	static void parent(Widget);

protected:

	Widget toggle_;
	Boolean active_;

private:

	searchable(const searchable&);
	searchable& operator=(const searchable&);

	static Widget parent_;
};

inline void destroy(searchable**) {}
#endif
