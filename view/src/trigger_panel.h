#ifndef trigger_panel_H
#define trigger_panel_H
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

#include "uitriggers.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef node_window_H
#include "node_window.h"
#endif

#ifndef depend_H
#include "depend.h"
#endif

class layout;

class trigger_panel : public panel, 
                      public depend,
	              public triggers_form_c , 
	              public triggers_menu_c {
public:
	trigger_panel(panel_window&);

	~trigger_panel(); // Change to virtual if base class

	virtual void create (Widget parent, char *widget_name = NULL);
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual const char* name() const { return "Triggers"; }
	Widget  menus(Widget);
	virtual Widget widget() { return triggers_form_c::xd_rootwidget(); }

	Boolean triggers()  { return triggers_;  }
	Boolean triggered() { return triggered_; }
	Boolean extended()  { return depend_;    }

	Widget infoMenu()    { return info_menu_; }
	Widget linkMenu()    { return link_menu_; }

	void   showDependWindow();
	void   hideDependWindow();
	Widget dependHyperText();

private:

	trigger_panel(const trigger_panel&);
	trigger_panel& operator=(const trigger_panel&);
	
	Boolean full_;
	Boolean triggers_;
	Boolean triggered_;
	Boolean depend_;
	layout* layout_;

	virtual void fullCB( Widget, XtPointer );
	virtual void triggeredCB( Widget, XtPointer );
	virtual void triggersCB( Widget, XtPointer );
	virtual void dependCB( Widget, XtPointer );
	virtual void entryCB( Widget, XtPointer );
	virtual void hyperCB( Widget, XtPointer );
	virtual void reachCB( Widget, XtPointer );

	virtual void linkCB( Widget, XtPointer );
};

inline void destroy(trigger_panel**) {}
#endif
