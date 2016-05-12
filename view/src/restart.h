#ifndef restart_H
#define restart_H

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

#include "node_alert.h"

class node;

class restart : public node_alert<restart> {
public:

// -- Exceptions
	// None

// -- Contructors

	restart();

// -- Destructor

	~restart(); // Change to virtual if base class

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

	restart(const restart&);
	restart& operator=(const restart&);

// -- Methods


// -- Overridden methods

	virtual bool keep(node*);

// -- Class members


// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const restart& p)
	//	{ p.print(s); return s; }

};

inline void destroy(restart**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(restart);
//#endif

#endif
