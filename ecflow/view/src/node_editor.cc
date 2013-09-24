//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#include "node_editor.h"

#ifndef translator_H
#include "translator.h"
#endif

#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>
#include "xec.h"

template<class T>
static void node_editor_set(node_editor& e,const char* name, const T& t)
{
	str v = translator<T,str>()(t);
	e.set(name,v);
}

template<class T>
static void node_editor_get(node_editor& e,const char* name, T& t)
{
	str v;
	e.get(name,v);
	t = translator<str,T>()(v);
}

void node_editor::set(const char* name,int value)
{
	node_editor_set(*this,name,value);
}

void node_editor::get(const char* name,int& value)
{
	node_editor_get(*this,name,value);
}

void node_editor::set(const char* name,const str& value)
{
	Widget w = find(name); if(!w) return;
	if(XmIsLabel(w))     xec_SetLabel(w,(char*)value.c_str());
	if(XmIsText(w))      XmTextSetString(w,(char*)value.c_str());
	if(XmIsTextField(w)) XmTextSetString(w,(char*)value.c_str());
}

void node_editor::get(const char* name,str& value)
{
	Widget w = find(name); if(!w) return;
	char* p = XmTextGetString(w);
	value = str(p);
	XtFree(p);
}
