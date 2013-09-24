//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include <stdio.h>
#include <stdarg.h>
#include "node_alert.h"
#include "gui.h"
#include "ecflowview.h"
#include "node.h"
#include "host.h"
#include "selection.h"
#include "collector.h"
#include <Xm/List.h>
extern "C" {
#include "xec.h"
}

template<class T>
node_alert<T>::node_alert(const char* title,int bg) :
  alert_(getenv("ecflow_view_alert"))
  , title_(title)
  , bg_ (bg)
{
  create(gui::top());
  set_menu(title);
  XtVaSetValues(_xd_rootwidget,XmNtitle,title,NULL);
  xec_SetLabel(label_,title);
  if(bg != -1) {
      XtVaSetValues(label_,XmNbackground,gui::colors(bg),NULL);
      /* XtVaSetValues(form_,XmNforeground,gui::colors(bg),0); */     
      /* XtVaSetValues(list_,XmNbackground,gui::colors(bg),0); */
      /* XtVaSetValues(list_,XmNforeground,gui::colors(bg),0); */
    }
}


template<class T>
node_alert<T>::~node_alert()
{}


template<class T>
void node_alert<T>::browseCB(Widget,XtPointer data)
{
    XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
	selection::notify_new_selection(find(cb->item));
}



template<class T>
void node_alert<T>::clearCloseCB(Widget,XtPointer)
{
	reset();
	XtUnmanageChild(form_);
}

template<class T>
void node_alert<T>::closeCB(Widget,XtPointer)
{
	XtUnmanageChild(form_);
}

template<class T>
void node_alert<T>::collectCB(Widget,XtPointer)
{
	XmString* items = 0;
	int count = 0;

	XtVaGetValues(list_,XmNitems,&items,XmNitemCount,&count,NULL);

	for(int i = 0; i < count ; i++)
	{
		node *n = find(items[i]);
		if(n) collector::show(*n);
	}
}
  
template<class T>
void node_alert<T>::notify_system(node* n) {
#ifdef linux
/*
  export ecflow_view_alert=1
  notify-send -i 'dialog-information' 'Summary' \
    '<b><font color=red>Message body.'
 */
  if(0 && alert_) {
    char buff[1024];
    const char *cmd = "kdialog --title ecFlowview::%s --passivepopup '<b><font color=%s> %s' 5; %s";
    const char *sound = "play -q /usr/share/xemacs/xemacs-packages/etc/sounds/boing.wav";
    snprintf(buff, 1024, cmd, 
	     title_.c_str(),
	     bg_ == STATUS_ABORTED ? "red" : "black",
	     n ? name(n) : "",
	     bg_ == STATUS_ABORTED ? sound : ""
	     );
    system(buff);
  }
#endif
  }
