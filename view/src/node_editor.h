#ifndef node_editor_H
#define node_editor_H
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

// 

#ifndef str_H
#include "str.h"
#endif

#ifndef editor_H
#include "editor.h"
#endif


class node_editor : public editor {
public:

// -- Exceptions
	// None

// -- Contructors

	node_editor() {}

// -- Destructor

	virtual ~node_editor() {} // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

	virtual void set(const char*,const str&);
	virtual void get(const char*,str&);

	virtual void set(const char*,int);
	virtual void get(const char*,int&);

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

	node_editor(const node_editor&);
	node_editor& operator=(const node_editor&);

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

	//friend ostream& operator<<(ostream& s,const node_editor& p)
	//	{ p.print(s); return s; }

};

inline void destroy(node_editor**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(node_editor);
//#endif

#endif
