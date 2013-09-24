#ifndef collector_H
#define collector_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#ifndef window_H
#include "window.h"
#endif

#ifndef uicollector_H
#include "uicollector.h"
#endif

#ifndef singleton_H
#include "singleton.h"
#endif

#ifndef node_list_H
#include "node_list.h"
#endif

#ifndef runnable_H
#include "runnable.h"
#endif

#ifndef array_H
#include "array.h"
#endif

#ifndef str_H
#include "str.h"
#endif

#ifndef xmstring_H
#include "xmstring.h"
#endif

class node;

class collector : public collector_shell_c, public window,
	public runnable,
	public singleton<collector>, public node_list {
public:

	collector();

	~collector(); // Change to virtual if base class

// -- Overridden methods

	virtual Widget shell() { return _xd_rootwidget; }
	virtual Widget list() { return list_; }
	virtual Widget form() { return form_; }

// -- Class members

	static void show(node&);

private:

	collector(const collector&);
	collector& operator=(const collector&);

	str        cmd_;
	array<xmstring> nodes_;
	int        next_;

	void update();
	void send(const char*);

	void run();

	virtual void closeCB( Widget, XtPointer );
	virtual void applyCB( Widget, XtPointer );
	virtual void removeCB( Widget, XtPointer );
	virtual void noneCB( Widget, XtPointer );
	virtual void allCB( Widget, XtPointer );
	virtual void selectCB( Widget, XtPointer );
	virtual void entryCB( Widget, XtPointer );
	virtual void stopCB( Widget, XtPointer );

	virtual bool keep(node*);

};

inline void destroy(collector**) {}
#endif
