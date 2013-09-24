#ifndef colors_prefs_H
#define colors_prefs_H
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

#ifndef prefs_H
#include "prefs.h"
#endif

#ifndef uicolors
#include "uicolors.h"
#endif

class colors_prefs : public prefs, public colors_form_c {
public:

	colors_prefs() {}

	~colors_prefs() {}

	virtual Widget widget() { return _xd_rootwidget; }

private:

	colors_prefs(const colors_prefs&);
	colors_prefs& operator=(const colors_prefs&);

	virtual void changedCB( Widget w, XtPointer ) { pref_editor::changed(w); }
	virtual void useCB( Widget w, XtPointer )     { pref_editor::use(w);     }

	virtual void create(Widget w,char*);
};

inline void destroy(colors_prefs**) {}
#endif
