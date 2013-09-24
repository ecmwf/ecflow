#ifndef fonts_prefs_H
#define fonts_prefs_H
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


// Headers
// #ifndef   machine_H
// #include <machine.h>
// #endif

// Forward declarations

// class ostream;
// typedef class _Pvts os_typespec; // Remove if not persistant

// 

#ifndef prefs_H
#include "prefs.h"
#endif

#ifndef uifonts
#include "uifonts.h"
#endif


class fonts_prefs : public prefs, public fonts_form_c {
public:

// -- Exceptions
	// None

// -- Contructors

	fonts_prefs() {}

// -- Destructor

	~fonts_prefs() {}
// -- Convertors
	// None

// -- Operators
	// None

// -- Methods
	// None

// -- Overridden methods

	virtual Widget widget() { return _xd_rootwidget; }

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

	fonts_prefs(const fonts_prefs&);
	fonts_prefs& operator=(const fonts_prefs&);

// -- Members
	// None

// -- Methods
	// None

// -- Overridden methods
	// None
	virtual void changedCB( Widget w, XtPointer ) { pref_editor::changed(w); }
	virtual void useCB( Widget w, XtPointer )     { pref_editor::use(w);     }

	virtual void create(Widget w,char*);

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const fonts_prefs& p)
	//	{ p.print(s); return s; }

};

inline void destroy(fonts_prefs**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(fonts_prefs);
//#endif

#endif
