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

#include "mail.h"
#include "host.h"
#include "runnable.h"
#include <Xm/List.h>
#include <Xm/Text.h>
#include "gui.h"
#include "extent.h"

extern "C" {
#include "xec.h"
}


class mail_user : public extent<mail_user> {
	char *host_;
	char *user_;
	bool mark_;
public:

	mail_user(const char* h,const char* u):
		host_(XtNewString(h)),
		user_(XtNewString(u)),
		mark_(true)
	{
	}

	~mail_user()
	{
		XtFree(host_);
		XtFree(user_);
	}
	
	static void add(mail&,const char*,const char*);
	static void remove(mail&,const char*);

	static void mark();
	static void sweep(mail&,const char*);
};

void mail_user::mark()
{
	mail_user *p = first();
	while(p)
	{
		p->mark_ = false;
		p = p->next();
	}
}

void mail_user::add(mail& m,const char* h,const char* u)
{
	mail_user *p = first();
	while(p)
	{
		if(strcmp(p->host_,h) == 0 && strcmp(p->user_,u) == 0)
		{
			p->mark_ = true;
			return;
		}
		p = p->next();
	}
	new mail_user(h,u);
	m.add(h,u);
}

void mail_user::remove(mail& m,const char* h)
{
	mail_user *p = first();
	while(p)
	{
		mail_user* n = p->next();
		if(strcmp(p->host_,h) == 0)
		{
			m.remove(p->host_,p->user_);
			delete p;
		}
		p = n;
	}
}

void mail_user::sweep(mail& m,const char* h)
{
	mail_user *p = first();
	while(p)
	{
		mail_user* n = p->next();
		if(strcmp(p->host_,h) == 0 && !p->mark_)
		{
			m.remove(p->host_,p->user_);
			delete p;
		}
		p = n;
	}
}

mail& mail::instance()
{
	static mail *m = new mail();
	return *m;
}

mail::mail():
	timeout(1)
{
	create(gui::top());
}

mail::~mail()
{
}

void mail::run()
{
	//printf("mail::run\n");
	host::check_all_mail();
	drift(1,3600*24);
}

void mail::recieved(host* h,std::list< std::string >&l,bool show)
{
	instance().new_mail(h,l,show);
}

class show_mail : public runnable {
	Widget widget_;
	void run()  { XtManageChild(widget_); disable(); gui::raise(); }
public:
	show_mail() : widget_(0) {}
	void show(Widget w) { widget_ = w; enable(); }
};


void mail::add(const char* buf)
{
	long len = XmTextGetLastPosition(text_);
    XmTextSetInsertionPosition(text_,len);
	XmTextReplace(text_,len,len,(char*)buf);
	len += strlen(buf);
	XmTextSetInsertionPosition(text_,len);
	XmTextShowPosition(text_,len);
}


void mail::new_mail(host* h,std::list<std::string>& l,bool show)
{
	mail_user::mark();

	static show_mail s;
	observe(h);
	if(show) {
		s.show(form_);
		enable();
	}

	mail_user::sweep(*this,h->name());
}

void mail::sendCB(Widget,XtPointer)
{
	int count;
	XtVaGetValues(list_,XmNselectedItemCount,&count,NULL);
	if(count == 0)
	{
		/* xec_ListSelectAll(list_); */
		/* XtVaGetValues(list_,XmNselectedItemCount,&count,0); */
		gui::error("No recipient selected");
		return;
	}

	XmString *items;
	XtVaGetValues(list_,XmNselectedItems,&items,NULL);

	char* p = XmTextGetString(input_);
	XmTextSetString(input_,"");	

	for(int i = 0 ; i < count; i++)
	{
		char *u = xec_GetString(items[i]);
		char *q = u;
		while(*q && *q != '@') q++;
		*q = 0;

		(void)host::find(q+1);
		XtFree(u);
	}

	add(p);
	add("\n");

	XtFree(p);
	run();
	frequency(1);
}

void mail::closeCB(Widget,XtPointer)  
{
	disable();
	XtUnmanageChild(form_);
}

void mail::gone(observable* h)
{
	mail_user::remove(*this,((host*)h)->name());
}

void mail::add(const char* h,const char* u)
{
	char buf[1024];
	sprintf(buf,"%s@%s",u,h);
	xec_AddListItem(list_,buf);
}

void mail::remove(const char* h,const char* u)
{
	char buf[1024];
	sprintf(buf,"%s@%s",u,h);
	xec_RemoveListItem(list_,buf);
}


void mail::login(const char* n)
{
  // FILL recieved(host::find(n),0,false);	
}

void mail::logout(const char* n)
{
	instance().gone(host::find(n));
}

IMP(mail_user)
