#ifndef passwrd_H
#define passwrd_H

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


#include "uipasswd.h"
#include "dialog.h"

class str;

class passwrd : public dialog<passwrd,passwd_shell_c> {
public:

// -- Exceptions
	// None

// -- Contructors

	passwrd();

// -- Destructor

	~passwrd(); // Change to virtual if base class

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

	static Boolean ask(const str&,str&,str&);

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

	passwrd(const passwrd&);
	passwrd& operator=(const passwrd&);



// -- Methods

	Boolean prompt(const str&,str&,str&);

// -- Overridden methods

	virtual void modifyCB( Widget, XtPointer );

// -- Class members


// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const passwrd& p)
	//	{ p.print(s); return s; }

};

inline void destroy(passwrd**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(passwrd);
//#endif

#endif
