/*
** Generated by X-Designer
*/
/*
**LIBS: -lXm -lXt -lX11
*/

#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>


#include "uiuser.h"

user_form_p user_form = (user_form_p) NULL;



void user_form_c::create (Widget parent, char *widget_name)
{
	Widget children[3];      /* Children to manage */
	Arg al[64];                    /* Arg List */
	register int ac = 0;           /* Arg Count */
	Widget frame2 = (Widget)NULL;
	Widget label2 = (Widget)NULL;
	Widget radioBox1 = (Widget)NULL;
	Widget toggle1 = (Widget)NULL;
	Widget toggle2 = (Widget)NULL;
	Widget toggle3 = (Widget)NULL;

	if ( !widget_name )
		widget_name = "User level";

	XtSetArg(al[ac], XmNautoUnmanage, FALSE); ac++;
	user_form = XmCreateForm ( parent, widget_name, al, ac );
	ac = 0;
	_xd_rootwidget = user_form;
	XtSetArg(al[ac], XmNautoUnmanage, FALSE); ac++;
	form_ = XmCreateForm ( user_form, "form_", al, ac );
	ac = 0;
	frame2 = XmCreateFrame ( form_, "frame2", al, ac );
	XtSetArg(al[ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
	label2 = XmCreateLabel ( frame2, "User level", al, ac );
	ac = 0;
	radioBox1 = XmCreateRadioBox ( frame2, "user_level", al, ac );
	toggle1 = XmCreateToggleButtonGadget ( radioBox1, "Normal", al, ac );
	toggle2 = XmCreateToggleButtonGadget ( radioBox1, "Operator", al, ac );
	toggle3 = XmCreateToggleButtonGadget ( radioBox1, "Administrator", al, ac );

	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetValues ( form_,al, ac );
	ac = 0;

	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtopOffset, 5); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftOffset, 5); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightOffset, 5); ac++;
	XtSetValues ( frame2,al, ac );
	ac = 0;

	XtAddCallback (toggle1, XmNvalueChangedCallback,&user_form_c:: changedCB, (XtPointer) this);
	XtAddCallback (toggle2, XmNvalueChangedCallback,&user_form_c:: changedCB, (XtPointer) this);
	XtAddCallback (toggle3, XmNvalueChangedCallback,&user_form_c:: changedCB, (XtPointer) this);
	children[ac++] = toggle1;
	children[ac++] = toggle2;
	children[ac++] = toggle3;
	XtManageChildren(children, ac);
	ac = 0;
	children[ac++] = label2;
	children[ac++] = radioBox1;
	XtManageChildren(children, ac);
	ac = 0;
	children[ac++] = frame2;
	XtManageChildren(children, ac);
	ac = 0;
	children[ac++] = form_;
	XtManageChildren(children, ac);
}

void user_form_c::useCB( Widget widget, XtPointer client_data, XtPointer call_data )
{
	user_form_p instance = (user_form_p) client_data;
	instance->useCB ( widget, call_data );
}

void user_form_c::changedCB( Widget widget, XtPointer client_data, XtPointer call_data )
{
	user_form_p instance = (user_form_p) client_data;
	instance->changedCB ( widget, call_data );
}
