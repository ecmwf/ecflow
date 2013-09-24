#ifndef edit_meter_H
#define edit_meter_H

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

// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant


#include "uiedit_meter.h"

#ifndef panel_H
#include "panel.h"
#endif

// 

class edit_meter : public panel, public edit_meter_form_c {
public:

// -- Exceptions
	// None

// -- Contructors

	edit_meter(panel_window&);

// -- Destructor

	~edit_meter(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	// None

// -- Overridden methods

	virtual const char* name() const { return "Edit"; }
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }
	virtual Widget tools() { return tools_; }


// -- Class members
	// None

// -- Class methods
	// None

	// Uncomment for persistent, remove otherwise
	// static os_typespec* get_os_typespec();

protected:

// -- Members
	// None

// -- Methods
	
	// void print(ostream&) const; // Change to virtual if base class	

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

private:

// No copy allowed

	edit_meter(const edit_meter&);
	edit_meter& operator=(const edit_meter&);

// -- Members
	bool loading_;

// -- Methods
	// None

// -- Overridden methods

	virtual void applyCB(Widget,XtPointer);
	virtual void changedCB(Widget,XtPointer);

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const edit_meter& p)
	//	{ p.print(s); return s; }

};

inline void destroy(edit_meter**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(edit_meter);
//#endif


#endif
