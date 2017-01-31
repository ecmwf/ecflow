#ifndef servers_prefs_H
#define servers_prefs_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#ifndef prefs_H
#include "prefs.h"
#endif

#ifndef uiservers
#include "uiservers.h"
#endif

#ifndef str_H
#include "str.h"
#endif

#ifndef array_H
#include "array.h"
#endif

#ifndef Singleton_H
#include "singleton.h"
#endif

class servers_prefs : public singleton<servers_prefs>, public prefs
  , public servers_form_c {
public:

	servers_prefs() : changing_(false) {}

	~servers_prefs() {}

	void check_remove();

	virtual Widget widget() { return _xd_rootwidget; }

	static void add_host(const std::string&);
private:

	servers_prefs(const servers_prefs&);
	servers_prefs& operator=(const servers_prefs&);

	array<str> hosts_;
	bool       changing_;
	str        current_;

	int number();
	str name();
	str machine();

	void add(str&);
	void build_list();

	virtual void browseCB( Widget w, XtPointer );
	virtual void serversCB( Widget w, XtPointer );
	virtual void addCB( Widget w, XtPointer );
	virtual void removeCB( Widget w, XtPointer );
	virtual void updateCB( Widget w, XtPointer );
	virtual void changedCB( Widget w, XtPointer );
	virtual void create(Widget w,char*);
};

inline void destroy(servers_prefs**) {}

#endif
