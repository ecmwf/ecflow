/*
** Generated by X-Designer
*/
#ifndef _uinode_alert_h
#define _uinode_alert_h

#define XD_MOTIF

#include <xdclass.h>

class node_alert_shell_c: public xd_XmDialog_c {
public:
	virtual void create (Widget parent, char *widget_name = NULL);
protected:
	Widget node_alert_shell;
	Widget form_;
	Widget list_;
	Widget close_;
	Widget clear_;
	Widget label_;
public:
	static void collectCB( Widget, XtPointer, XtPointer );
	virtual void collectCB( Widget, XtPointer ) = 0;
	static void clearCloseCB( Widget, XtPointer, XtPointer );
	virtual void clearCloseCB( Widget, XtPointer ) = 0;
	static void closeCB( Widget, XtPointer, XtPointer );
	virtual void closeCB( Widget, XtPointer ) = 0;
	static void browseCB( Widget, XtPointer, XtPointer );
	virtual void browseCB( Widget, XtPointer ) = 0;
};

typedef node_alert_shell_c *node_alert_shell_p;


extern node_alert_shell_p node_alert_shell;


#endif