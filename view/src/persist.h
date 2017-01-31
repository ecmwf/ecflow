#ifndef persist_H
#define persist_H
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
#ifdef NO_BOOL
#include "bool.h"
#endif

struct sms_list;

class persist {
public:

// -- Exceptions
	// None

// -- Contructors

	persist(const char* kind,const char* name);

// -- Destructor

	~persist(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

	void set(const char*,const char*);
	void set(const char*,int);
	void set(const char*,sms_list*);

	bool get(const char*,char*);
	bool get(const char*,int&);
	bool get(const char*,sms_list*&);

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

	persist(const persist&);
	persist& operator=(const persist&);

// -- Members

	const char* kind_;
	const char* name_;
	FILE *f_;
	bool write_;

// -- Methods

	bool open(bool);
	void close();
	const char* read(const char*);

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const persist& p)
	//	{ p.print(s); return s; }

};

inline void destroy(persist**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(persist);
//#endif

#endif
