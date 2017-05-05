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

#include <stdlib.h>
#include <inttypes.h>
#include <string>
#include <list>

#ifndef option_H
#include "option.h"
#endif

#ifndef translator_H
#include "translator.h"
#endif

#ifndef choice_H
#include "choice.h"
#endif


#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#ifdef NO_BOOL
#include "bool.h"
#endif

#include "ecflow.h"

template<class T>
option<T>::option(configurable* o,const str& name,const T& val):
	resource(o,name,translator<T,str>()(val))
{
	value_ = translator<str,T>()(get());
}

template<class T>
option<T>::~option()
{
}

template<class T>
bool option<T>::changed()
{
	T old = value_;
	value_ = translator<str,T>()(get());
	return old != value_;
}


template<class T>
void option<T>::put(const T& v)
{
	set(translator<T,str>()(v));
}

//==========================================================

inline
void init_widget(Widget w,const str& s)
{
	XmTextSetString(w,(char*)s.c_str());
}

inline
void init_widget(Widget w,int n)
{
	str s = translator<int,str>()(n);
	XmTextSetString(w,(char*)s.c_str());
}

inline
void init_widget(Widget w,long n)
{
	str s = translator<long,str>()(n);
	XmTextSetString(w,(char*)s.c_str());
}

inline
void init_widget(Widget w,uint64_t n)
{
	str s = translator<uint64_t,str>()(n);
	XmTextSetString(w,(char*)s.c_str());
}

inline
void init_widget(Widget w,bool v)
{
	XmToggleButtonSetState(w,v,False);
}

inline
void init_widget(Widget,std::vector< std::string >&)
{
} 

inline
void init_widget(Widget w,const choice& c)
{
	WidgetList wl = 0;
	int count = 0;

    XtVaGetValues(w, XmNchildren,&wl, XtNnumChildren,&count, NULL);

	for(int i = 0; i < count; i++)
		XmToggleButtonSetState(wl[i],i == c,False);	
}


inline
str read_widget(Widget w,const str&)
{
	char* p = XmTextGetString(w);
	str b(p);
	XtFree(p);
	return b;
}

inline 
int read_widget(Widget w,int)
{
	char* p = XmTextGetString(w);
	int n = atol(p);
	XtFree(p);
	return n;
}

inline 
long read_widget(Widget w,long)
{
	char* p = XmTextGetString(w);
	long n = atol(p);
	XtFree(p);
	return n;
}

inline 
long read_widget(Widget w,uint64_t)
{
	char* p = XmTextGetString(w);
	uint64_t n = atoll(p);
	XtFree(p);
	return n;
}

inline
bool read_widget(Widget w,bool)
{
	return XmToggleButtonGetState(w);
}

inline
ecf_list* read_widget(Widget,ecf_list*)
{
  return 0;
} 

inline
std::vector< std::string > read_widget(Widget,std::vector< std::string >&)
{
  std::vector< std::string > out;
  return out;
}

inline
int read_widget(Widget w,const choice& c)
{
	WidgetList wl = 0;
	int count = 0;

	XtVaGetValues(w, XmNchildren,&wl, XtNnumChildren,&count, NULL);

	for(int i = 0; i < count; i++)
		if(XmToggleButtonGetState(wl[i]) )
			return i;
	return c;
}

template<class T>
void option<T>::initWidget(Widget w)
{
	init_widget(w,value_);
}

template<class T>
bool option<T>::readWidget(Widget w)
{
	T b = read_widget(w,value_);
	bool x = (b != value_);
	if(x) put(b);
	return x;
}
