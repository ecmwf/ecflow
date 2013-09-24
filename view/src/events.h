//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#ifndef events_H
#define events_H

// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

// 

#ifndef array_H
#include "array.h"
#endif


class events {
public:

// -- Exceptions
	// None

// -- Contructors

	events() {}

// -- Destructor

	~events() {}

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

	int count()       { return time_.count(); }
	int time(int i)   { return time_[i]; }
	int status(int i) { return status_[i]; }

	void add(int t,int s) { time_.add(t); status_.add(s); }

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

	events(const events&);
	events& operator=(const events&);

// -- Members
	
	array<int> time_;
	array<int> status_;

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const events& p)
	//	{ p.print(s); return s; }

};

inline void destroy(events**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(events);
//#endif

#endif
