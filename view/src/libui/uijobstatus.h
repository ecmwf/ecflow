/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #1 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2016 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#ifndef _uijobstatus_h
#define _uijobstatus_h

#define XD_MOTIF

#include <xdclass.h>

class jobstatus_form_c: public xd_XmForm_c {
public:
	virtual void create (Widget parent, char *widget_name = NULL);
protected:
	Widget jobstatus_form;
	Widget text_;
	Widget name_;
	Widget tools_;
public:
	static void searchCB( Widget, XtPointer, XtPointer );
	virtual void searchCB( Widget, XtPointer ) = 0;
	static void externalCB( Widget, XtPointer, XtPointer );
	virtual void externalCB( Widget, XtPointer ) = 0;
	static void updateCB( Widget, XtPointer, XtPointer );
	virtual void updateCB( Widget, XtPointer ) = 0;
};

typedef jobstatus_form_c *jobstatus_form_p;
class script;
typedef script *script_p;


extern script_p jobstatus_form;


#endif
