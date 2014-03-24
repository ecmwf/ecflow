#ifndef gui_H
#define gui_H
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


#include "ecflowview.h"
#include <Xm/Xm.h>
#include <string>

class resource;
class interface;
class node;

Pixel pixel(const char* name);

class gui {
public:

	gui() {}

	static void clear();
	static void message(const char* fmt,...);
	static void watch(Boolean);

	static void add_host(const std::string&);
	static void rename_host(const std::string&,const std::string&);
	static void remove_host(const std::string&);

	static void login(const char*);
	static void logout(const char*);

	static Widget top();
	static Widget trees();
	static Widget windows();

	static void raise();

	//--------------------------------------

	static void       changed(resource&);

	static GC         blackGC();
	static GC         blueGC();
	static GC         redGC();

	static XmFontList smallfont();
	static XmFontList fontlist();
	static XmFontList tinyfont();
	static Pixel      colors(unsigned int);
	static GC         colorGC(unsigned int);

	static void set_interface(interface*);

	//------------------------------------------------------

	static bool visible();

	static void error(const char*,...);
	void error(ecf_list *l);
	static void syserr(const char*);

private:

	gui(const gui&);
	gui& operator=(const gui&);

};

inline void destroy(gui**) {}
#endif
