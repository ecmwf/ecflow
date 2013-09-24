#ifndef html_lister_H
#define html_lister_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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

// 

#include "node.h"
#include "text_lister.h"

class html_lister : public text_lister {
public:

// -- Exceptions
	// None

// -- Contructors

	html_lister(node*);

// -- Destructor

	virtual ~html_lister(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	
	virtual void line(const char*) = 0;

// -- Overridden methods

	void push(node* n);
	void push(const char* p,...);
	void endline();
	void cancel();
	node* source() const { return node_; }

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

	html_lister(const html_lister&);
	html_lister& operator=(const html_lister&);

// -- Members

	node*  node_;
	int    nodes_;
	int    cancels_;
	char   buf_[1024];

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const html_lister& p)
	//	{ p.print(s); return s; }

};

inline void destroy(html_lister**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(html_lister);
//#endif

#endif
