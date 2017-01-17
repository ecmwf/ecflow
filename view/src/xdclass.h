#ifndef XDCLASS_H
#define XDCLASS_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#ifdef __cplusplus

#include <Xm/Xm.h>
#include <stdlib.h>
#pragma GCC diagnostic ignored "-Wwrite-strings"

class xd_base_c
{
    public:
	xd_base_c() {_xd_rootwidget=NULL;}
	Widget xd_rootwidget() const {return _xd_rootwidget;}
    protected:
	Widget _xd_rootwidget;
    private:
	void operator=(xd_base_c&); // No assignment
	// Certain C++ compilers (eg gcc 2.5) require there to be an
	// implementation of the copy constructor. If your application
	// fails to link try using the second version of the constructor
	//xd_base_c(xd_base_c&) ;           // No default copy
	xd_base_c(xd_base_c&) { abort();}   // No default copy
};

class xd_XtWidget_c: public xd_base_c
{
    public:
	xd_XtWidget_c();
	void SetValue(String name, XtArgVal value);
	void SetValues(ArgList args, Cardinal num_args);
	void VaSetValues(String name,...);
	void GetValue(String name, void *value);
	void GetValues(ArgList args, Cardinal num_args);
	void VaGetValues(String name,...);
	void Map();
	void Unmap();
	virtual void xd_enable();
	virtual void xd_disable();
	virtual void xd_destroy();
};

class xd_TopLevelShell_c: public xd_XtWidget_c
{
};

class xd_ApplicationShell_c: public xd_TopLevelShell_c
{
    public:
	void xd_exit(int status=0);
	void Realize();
};

class xd_ChildWidget_c: public xd_XtWidget_c
{
    public:
	virtual void xd_show()=0;
	virtual void xd_hide()=0;
	void Manage();
	void Unmanage();
};

class xd_XmDialog_c: public xd_ChildWidget_c
{
    public:
	xd_XmDialog_c();
	virtual void xd_show();
	virtual void xd_hide();
	void Raise();
    protected:
	Widget xd_getchildwidget();
	Widget xd_childwidget;
};

class xd_NonShellWidget_c: public xd_ChildWidget_c
{
    public:
	virtual void xd_show();
	virtual void xd_hide();
};

class xd_XmLabel_c: public xd_NonShellWidget_c
{
};

class xd_XmCascadeButton_c: public xd_NonShellWidget_c
{
};

class xd_XmDrawnButton_c: public xd_NonShellWidget_c
{
};

class xd_XmPushButton_c: public xd_NonShellWidget_c
{
};

class xd_XmToggleButton_c: public xd_NonShellWidget_c
{
};

class xd_XmArrowButton_c: public xd_NonShellWidget_c
{
};

class xd_XmList_c: public xd_NonShellWidget_c
{
};

class xd_XmScrollBar_c: public xd_NonShellWidget_c
{
};

class xd_XmSeparator_c: public xd_NonShellWidget_c
{
};

class xd_XmText_c: public xd_NonShellWidget_c
{
};

class xd_XmTextField_c: public xd_NonShellWidget_c
{
};

class xd_XmBulletinBoard_c: public xd_NonShellWidget_c
{
};

class xd_XmRowColumn_c: public xd_NonShellWidget_c
{
};

class xd_XmRadioBox_c: public xd_NonShellWidget_c
{
};

class xd_XmDrawingArea_c: public xd_NonShellWidget_c
{
};

class xd_XmPanedWindow_c: public xd_NonShellWidget_c
{
};

class xd_XmFrame_c: public xd_NonShellWidget_c
{
};

class xd_XmScale_c: public xd_NonShellWidget_c
{
};

class xd_XmScrolledWindow_c: public xd_NonShellWidget_c
{
};

class xd_XmScrolledText_c: public xd_NonShellWidget_c
{
};

class xd_XmScrolledList_c: public xd_NonShellWidget_c
{
};

class xd_XmMainWindow: public xd_NonShellWidget_c
{
};

class xd_XmSelectionBox_c: public xd_NonShellWidget_c
{
};

class xd_XmFileSelectionBox_c: public xd_NonShellWidget_c
{
};

class xd_XmCommand_c: public xd_NonShellWidget_c
{
};

class xd_XmMessageBox_c: public xd_NonShellWidget_c
{
};

class xd_XmMainWindow_c: public xd_NonShellWidget_c
{
};

class xd_XmForm_c: public xd_NonShellWidget_c
{
};

class xd_XmMenuBar_c: public xd_NonShellWidget_c
{
};

class xd_XmPulldownMenu_c: public xd_NonShellWidget_c
{
};

class xd_XmPopupMenu_c: public xd_NonShellWidget_c
{
};

class xd_XmOptionMenu_c: public xd_NonShellWidget_c
{
};

#endif /*__cplusplus*/
#endif /*XDCLASS_H*/
