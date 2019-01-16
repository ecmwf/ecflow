#ifndef viewer_H
#define viewer_H
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


#ifdef NO_BOOL
#include "bool.h"
#endif

#ifndef input_H
#include "input.h"
#endif



// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

// 

class viewer : public input {
public:

// -- Exceptions
	// None

// -- Contructors

	viewer();

// -- Destructor

    virtual ~viewer();

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

     virtual bool show(const char*);
	 virtual void end(bool);

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

	viewer(const viewer&);
	viewer& operator=(const viewer&);

// -- Members

// -- Methods
	// None

// -- Overridden methods

     void ready(const char*);
     void done(FILE*);

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const viewer& p)
	//	{ p.print(s); return s; }

};

inline void destroy(viewer**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(viewer);
//#endif

#endif
