#ifndef users_H
#define users_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
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


#include "uiusers.h"

#ifndef panel_H
#include "panel.h"
#endif

// 

class users : public panel, public users_form_c {
public:

// -- Exceptions
	// None

// -- Contructors

	users(panel_window&);

// -- Destructor

	~users(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	// None

// -- Overridden methods

	virtual const char* name() const { return "Users"; }
	virtual void show(node&);
	virtual void clear();
	virtual Boolean enabled(node&);
	virtual Widget widget() { return users_form_c::xd_rootwidget(); }

	virtual void create (Widget parent, char *widget_name = NULL);

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

	users(const users&);
	users& operator=(const users&);

// -- Members


// -- Methods
	// None

// -- Overridden methods

	virtual void sendCB( Widget, XtPointer );

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const users& p)
	//	{ p.print(s); return s; }

};

inline void destroy(users**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(users);
//#endif


#endif
