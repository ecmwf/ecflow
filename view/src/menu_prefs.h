#ifndef menu_prefs_H
#define menu_prefs_H
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


#ifndef prefs_H
#include "prefs.h"
#endif

#ifndef uimenu
#include "uimenu.h"
#endif

class menu_prefs : public prefs, public menu_form_c {
public:
	menu_prefs() : changing_(false) {}

	~menu_prefs() {}

	void check_remove();

	virtual Widget widget() { return _xd_rootwidget; }

	static void add_host(const char*);

private:

	menu_prefs(const menu_prefs&);
	menu_prefs& operator=(const menu_prefs&);

	bool       changing_;

	void build_list();

	virtual void browseCB( Widget w, XtPointer );

	virtual void menuCB( Widget w, XtPointer );
	virtual void addCB( Widget w, XtPointer );
	virtual void removeCB( Widget w, XtPointer );
	virtual void updateCB( Widget w, XtPointer );
	virtual void changedCB( Widget w, XtPointer );

	virtual void create(Widget w,char*);
};

inline void destroy(menu_prefs**) {}
#endif
