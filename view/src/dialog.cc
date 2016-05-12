#ifndef dialog_H
#include "dialog.h"
#endif

#ifndef SVR4
#include <stdio.h>
#endif

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


extern "C" {
#include "xec.h"
}


template<class T,class U>
void dialog<T,U>::helpCB( Widget, XtPointer )
{
}

template<class T,class U>
void dialog<T,U>::cancelCB( Widget, XtPointer )
{
	ok_   = False;
	stop_ = True;
}

template<class T,class U>
void dialog<T,U>::okCB( Widget, XtPointer )
{
	ok_  = True;
	stop_ = True;
}


template<class T,class U>
Boolean dialog<T,U>::modal(const char* message,Boolean def_ok)
{
	XtVaSetValues(this->form_,
		XmNdefaultButtonType,
		(def_ok? XmDIALOG_OK_BUTTON : XmDIALOG_CANCEL_BUTTON),
		NULL);

	if(message)
		xec_SetLabel(this->label_,message);

	XtManageChild(this->form_);

	stop_ = False;

	XEvent  event_node;
	XtAppContext ac = XtWidgetToApplicationContext(this->form_);

	while(!stop_)
	{
		XtAppNextEvent(ac,&event_node);
		XtDispatchEvent(&event_node);
	}

	XtUnmanageChild(this->form_);
	
	return ok_;
}

template<class T,class U>
void dialog<T,U>::show()
{
	XtManageChild(this->form_);
}
