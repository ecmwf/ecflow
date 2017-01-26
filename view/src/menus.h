#ifndef menus_H
#define menus_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #9 $ 
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


#ifdef NO_BOOL
#include "bool.h"
#endif

#include <Xm/Xm.h>

class node;
class menu;
class item;
class action;
class flags;

class menus {
public:

	menus();
	~menus(); // Change to virtual if base class

	static void entryCB(Widget,XtPointer,XtPointer);

	static void show(Widget,XEvent*,node*);

	static void  root(menu*);
	static menu* chain(menu*,menu*);
	static item* chain(item*,item*);
	static menu* create(char*,item*);
	static item* create(flags*,flags*,char*,action*,char*,bool);
	static action* command(char*);
	static action* separator();
	static action* sub_menu();
	static action* internal( void (*)(node*) );
	static action* internal_a_b( void (*)(node*, const char*, const char*),
                                     const char *a, const char *b);
	static action* window(char*);

	static void  install(Widget,const char*);

	static void fillList(Widget); 

	static void realize();
	static void write();

	static int version(int rel, int maj, int min);

private:

	menus(const menus&);
	menus& operator=(const menus&);
};

inline void destroy(menus**) {}

int script_menus(node*, const char *cmd);

#endif
