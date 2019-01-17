#ifndef prefs_H
#define prefs_H
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


// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

// 

#ifndef extent_H
#include "extent.h"
#endif

#ifndef pref_editor_H
#include "pref_editor.h"
#endif


class prefs : public extent<prefs>, public pref_editor {
public:

// -- Exceptions
	// None

// -- Contructors

	prefs() {};

// -- Destructor


// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

	virtual void create(Widget,char* = 0) = 0;
	virtual Widget widget() = 0;


	void setup(Widget);

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods

	static void create_all(Widget);

	// Uncomment for persistent, remove otherwise
	// static os_typespec* get_os_typespec();

protected:

// -- Members


// -- Methods
	

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

private:

// No copy allowed

	prefs(const prefs&);
	prefs& operator=(const prefs&);

// -- Members

// -- Methods


// -- Overridden methods
	// None

	virtual Widget form()         { return widget(); }
	virtual configurable* owner();

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends


};

#endif
