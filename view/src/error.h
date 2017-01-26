#ifndef error_H
#define error_H
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


#include "uierror.h"
#include "dialog.h"

class error : public dialog<error,error_shell_c> {
public:

// -- Exceptions
	// None

// -- Contructors

	error();

// -- Destructor

	~error(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods

	static void show(const char*);

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

	error(const error&);
	error& operator=(const error&);

// -- Methods


// -- Overridden methods


// -- Class members


// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const error& p)
	//	{ p.print(s); return s; }

};

inline void destroy(error**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(error);
//#endif

#endif
