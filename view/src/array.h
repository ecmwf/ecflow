#ifndef array_H
#define array_H
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


#include <stdio.h>

// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

// 

template<class T> 
class array {
public:

// -- Exceptions
	// None

// -- Contructors

	array();
	array(const array<T>&);

// -- Destructor

	~array(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators

	array<T>& operator=(const array<T>&);

	T& operator[](int i ) { return values_[i]; }
	int count() { return count_; }

// -- Methods
	// None

	void add(const T&);
	void remove(const T&);
	void clear();

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

	int count_;
	int max_;
	T* values_;

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

	//friend ostream& operator<<(ostream& s,const array& p)
	//	{ p.print(s); return s; }

};


#include "array.cc"

#endif
