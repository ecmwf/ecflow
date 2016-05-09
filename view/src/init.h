#ifndef init_H
#define init_H
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


#ifndef extent_H
#include "extent.h"
#endif

// 

class init : public extent<init> {
public:

// -- Exceptions
	// None

// -- Contructors

	init();

// -- Destructor

	virtual ~init(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	
	virtual void run(int,char**) = 0;

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods

	static void initialize(int,char**);

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

	init(const init&);
	init& operator=(const init&);

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

	//friend ostream& operator<<(ostream& s,const init& p)
	//	{ p.print(s); return s; }

};

inline void destroy(init**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(init);
//#endif

#endif
