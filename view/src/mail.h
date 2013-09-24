#ifndef mail_H
#define mail_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#include<list>
#include "uimail.h"
#include "observer.h"
#include "timeout.h"

#ifdef NO_BOOL
#include "bool.h"
#endif

#include <string>

class host;

class mail : public observer, public mail_shell_c, public timeout {
public:

	mail();

	~mail(); // Change to virtual if base class

	void add(const char*,const char*); 
	void remove(const char*,const char*); 

	static void recieved(host*,std::list< std::string >&,bool = true);
	static void login(const char*);
	static void logout(const char*);

private:

	mail(const mail&);
	mail& operator=(const mail&);

	void new_mail(host* h,std::list<std::string>& l,bool show);
	  // void new_mail(host*,std::list< std::string >&,bool);
	  void add(const char*);

	virtual void run();
	virtual void adoption(observable*,observable*) {}
	virtual void notification(observable*) {}
	virtual void gone(observable*);

	static mail& instance();

	void sendCB(Widget,XtPointer);
	void closeCB(Widget,XtPointer);
};

inline void destroy(mail**) {}
#endif
