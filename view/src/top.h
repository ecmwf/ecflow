#ifndef top_H
#define top_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#include "uitop.h"
#include "timeout.h"
#include "interface.h"

class top : public top_shell_c, public timeout, public interface {
public:

	top();
	~top(); // Change to virtual if base class

	virtual void create (Display *display, char *app_name, 
			     int app_argc, char **app_argv, 
			     char *app_class_name = NULL);

	virtual void run();

	virtual void clear();
	virtual void message(const char*);
	virtual void watch(Boolean);

	virtual void add_host(const std::string&);
	virtual void remove_host(const std::string&);
	virtual void rename_host(const std::string&,const std::string&) {}

	virtual void login(const char*);
	virtual void logout(const char*);

	virtual Widget top_shell();
	virtual Widget trees();
	virtual Widget windows();

	virtual void error(const char*);
private:

	top(const top&);
	top& operator=(const top&);

	virtual void quitCB(Widget,XtPointer);
	virtual void serverCB(Widget,XtPointer);
	virtual void statusCB(Widget,XtPointer);
	virtual void windowCB(Widget,XtPointer);
	virtual void showCB(Widget,XtPointer);
	virtual void chatCB(Widget,XtPointer);
	virtual void windowsCB(Widget,XtPointer);
	virtual void searchCB(Widget,XtPointer);
	virtual void helpCB(Widget,XtPointer);
	virtual void prefCB(Widget,XtPointer);
	virtual void loginCB(Widget,XtPointer);
	virtual void releaseCB(Widget,XtPointer);
	virtual void snapshotCB(Widget,XtPointer);
};

inline void destroy(top**) {}
#endif
