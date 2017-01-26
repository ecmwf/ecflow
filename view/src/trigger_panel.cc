//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "trigger_panel.h"
#include "node.h"
#include "graph_layout.h"
#include "text_layout.h"
#include "selection.h"
#include "Hyper.h"
#include <Xm/ToggleB.h>

extern "C" {
#include "xec.h"
}

trigger_panel::trigger_panel(panel_window& w):
	panel(w),
	full_(True),
	triggers_(True),
	triggered_(True),
	depend_(False),
	layout_(0)
{
}

trigger_panel::~trigger_panel()
{
	delete (layout*)xec_GetUserData(tgraph_);
	delete (layout*)xec_GetUserData(ttext_);
}

void trigger_panel::create(Widget parent, char *widget_name )
{
	triggers_form_c::create(parent,widget_name);
	xec_SetUserData(tgraph_,layout_ = new graph_layout(*this,graph_));
	xec_SetUserData(ttext_,new text_layout(*this,text_));
}

void trigger_panel::clear()
{
	layout_->clear();
	hide();
}

void trigger_panel::show(node& n)
{
	layout_->show(n);
}

Boolean trigger_panel::enabled(node& n)
{
	return True;
}

Widget trigger_panel::menus(Widget bar)
{
	if(triggers_menu_c::xd_rootwidget() == 0)
		triggers_menu_c::create(bar,(char*)name());
	return triggers_menu_c::xd_rootwidget();
}

void trigger_panel::fullCB( Widget, XtPointer data)
{
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*) data;
	full_ = cb->set;
	update();
}

void trigger_panel::triggeredCB( Widget, XtPointer data)
{
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*) data;
	triggered_ = cb->set;
	update();
}

void trigger_panel::triggersCB( Widget, XtPointer data)
{
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*) data;
	triggers_ = cb->set;
	update();
}

void trigger_panel::dependCB( Widget, XtPointer data)
{
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct*) data;
	depend_ = cb->set;
	clear();
	update();
}

void trigger_panel::entryCB(Widget, XtPointer data)
{
	XmRowColumnCallbackStruct* cb = (XmRowColumnCallbackStruct*)data;

	if(XmToggleButtonGetState(cb->widget))
	{
		layout *l = (layout*)xec_GetUserData(cb->widget);
		XtUnmanageChild(layout_->layout_widget());
		XtManageChild(l->layout_widget());
		layout_ = l;
		if(get_node())
			l->show(*get_node());
		else
			l->clear();
	}
	hide();
}

void trigger_panel::hyperCB(Widget w,XtPointer data)
{
	panel::hyper(w,data);
}

void trigger_panel::reachCB(Widget w,XtPointer data)
{
	XmToggleButtonSetState(dependencies_button_,True,False);
	XmToggleButtonSetState(triggers_button_,True,False);
	XmToggleButtonSetState(triggered_button_,True,False);

	depend_ = triggers_ = triggered_ = true;
	clear();
	layout_->reach(get_node(),selection::current_node());
}

void trigger_panel::linkCB(Widget w,XtPointer data)
{
	XmRowColumnCallbackStruct* cb = (XmRowColumnCallbackStruct*)data;
	node* n = (node*)xec_GetUserData(cb->widget);
	layout_->selectNode(n);
}

void trigger_panel::showDependWindow()
{
	depend::raise(widget());
}

Widget trigger_panel::dependHyperText()
{
	depend::make(widget());
	return depend::hyper_;
}

void trigger_panel::hideDependWindow()
{
	depend::hide();
}

// static panel_maker<trigger_panel> maker(PANEL_TRIGGER);
