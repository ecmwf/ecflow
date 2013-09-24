#ifndef messages_H
#define messages_H
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


#include "uimessages.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef text_window_H
#include "text_window.h"
#endif

// 

class messages : public panel, public messages_form_c, public text_window {
public:

	messages(panel_window&);

	~messages(); // Change to virtual if base class

	virtual const char* name() const { return "Messages"; }
	virtual void show(node&);
	virtual void clear();
	virtual Widget widget() { return messages_form_c::xd_rootwidget(); }
	virtual Boolean enabled(node&);

	virtual Widget tools()  { return tools_; }
	virtual Widget text()   { return text_; }
	virtual void create (Widget parent, char *widget_name = 0 );



private:

	messages(const messages&);
	messages& operator=(const messages&);

	virtual void externalCB(Widget ,XtPointer ) 
		{ text_window::open_viewer();}

	virtual void searchCB(Widget ,XtPointer ) 
		{ text_window::open_search();}

	virtual bool can_print() { return true; }
	virtual bool can_save()  { return true; }
	virtual void print()     { text_window::print(); }
	virtual void save()      { text_window::save(); }
};

inline void destroy(messages**) {}
#endif
