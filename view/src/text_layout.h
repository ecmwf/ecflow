#ifndef text_layout_H
#define text_layout_H
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



#ifndef layout_H
#include "layout.h"
#endif

#ifndef observer_H
#include "observer.h"
#endif


// 

class text_layout : public layout, public observer {
public:

// -- Exceptions
	// None

// -- Contructors

	text_layout(trigger_panel&,Widget);

// -- Destructor

	~text_layout(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	// None


// -- Overridden methods

	virtual void show(node&);
	virtual void clear();
	virtual void reach(node*,node*);


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

	text_layout(const text_layout&);
	text_layout& operator=(const text_layout&);

// -- Members
	

// -- Methods


	void notification(observable*);
	void adoption(observable*,observable*);
	void gone(observable*);

// -- Overridden methods

	// From triigers_menu_c


// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const text_layout& p)
	//	{ p.print(s); return s; }

};

inline void destroy(text_layout**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(text_layout);
//#endif


#endif
