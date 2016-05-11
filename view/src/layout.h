#ifndef layout_H
#define layout_H

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

// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

// 

#include <Xm/Xm.h>

class node;
class trigger_panel;

class layout {
public:

// -- Exceptions
	// None

// -- Contructors

	layout(trigger_panel&,Widget);

// -- Destructor

	virtual ~layout(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

	virtual void clear() = 0;
	virtual void show(node&) = 0 ;
	virtual void reach(node*,node*) = 0;
	virtual void selectNode(node*) {}

	trigger_panel& owner() { return owner_; }
	Widget layout_widget() { return widget_; }

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

	// Uncomment for persistent, remove otherwise
	// static os_typespec* get_os_typespec();

protected:

// -- Members
	
	Widget widget_;
	trigger_panel& owner_;

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

	layout(const layout&);
	layout& operator=(const layout&);

// -- Members
	// None

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const layout& p)
	//	{ p.print(s); return s; }

};

inline void destroy(layout**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(layout);
//#endif

#endif
