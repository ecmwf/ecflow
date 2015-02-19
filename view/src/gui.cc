//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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

#include <stdarg.h>
#include <stdio.h>
#include "gui.h"
#include "top.h"
#include "str.h"
#include "option.h"
#include "host.h"
#include "globals.h"
#include "Str.hpp"

#include <string.h> /* strerror */
using namespace ecf;

static interface* intf_= 0;

// Colors

static resource* gui_resources[] = {
	new option<str>(globals::instance(),"color_black","black"),
	new option<str>(globals::instance(),"color_blue","blue"),
	new option<str>(globals::instance(),"color_red","red"),

	new option<str>(globals::instance(),"color_unknown",   "grey"),
	new option<str>(globals::instance(),"color_suspended", "orange"),
	new option<str>(globals::instance(),"color_complete",  "yellow"),
	new option<str>(globals::instance(),"color_queued",    "lightblue"),
	new option<str>(globals::instance(),"color_submitted", "turquoise"),
	new option<str>(globals::instance(),"color_active",    "green"),
	new option<str>(globals::instance(),"color_aborted",   "red"),
	new option<str>(globals::instance(),"color_shutdown",  "pink"),
	new option<str>(globals::instance(),"color_halted",    "violet"),

	new option<str>(globals::instance(),"color_meter_low",  "blue"),
	new option<str>(globals::instance(),"color_threshold",  "blue"),
	new option<str>(globals::instance(),"color_event",      "blue"),

	new option<str>(globals::instance(),"normal_font_plain", 
			"-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*"),

	new option<str>(globals::instance(),"normal_font_bold",  
			"-*-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*"),

	new option<str>(globals::instance(),"small_font_plain",  
			"-*-helvetica-medium-r-normal-*-11-*-*-*-*-*-*-*"),

	new option<str>(globals::instance(),"small_font_bold",   
			"-*-helvetica-bold-r-normal-*-11-*-*-*-*-*-*-*"),

	new option<str>(globals::instance(),"tiny_font_plain",  
			"-*-*-*-*-*-*-7-*-*-*-*-*-*-*"),

	new option<str>(globals::instance(),"tiny_font_bold",   
			"-*-*-bold-*-*-*-7-*-*-*-*-*-*-*"),
};

class tidy_gui_resources {
public:
	~tidy_gui_resources() {
		for(unsigned int i = 0; i < XtNumber(gui_resources) ; i++)
                  delete gui_resources[i];
	}
};

void split_msg(std::string& msg) {
  std::vector< std::string > lineTokens;
  Str::split(msg, lineTokens);
  msg.clear();
  for (size_t i=0; i<lineTokens.size(); ++i) {
    msg += lineTokens[i];
    if (i%10 == 0) msg += "\n";
    else msg += " ";
  }
}

#if !defined(_AIX)
static tidy_gui_resources tgr;
#endif

void gui::login(const char* host)
{
  if (intf_) intf_->login(host);
}

void gui::logout(const char* host)
{
  if (intf_) intf_->logout(host);
}

void gui::add_host(const std::string& host)
{
  if (intf_) intf_->add_host(host);
}

void gui::message(const char* fmt,...)
{
    char buf[1024];
    va_list   args;
    va_start(args,fmt);
    vsprintf(buf,fmt,args);
    intf_->message(buf);
    va_end(args);

    va_start(args,fmt);    
    va_end(args);
}

void gui::raise()
{
  Widget w = top();
  if(w && XtIsRealized(w))
    XMapRaised(XtDisplay(w),XtWindow(w));
}

Widget gui::top()
{
	return intf_->top_shell();
}

Widget gui::trees()
{
	return intf_->trees();
}

Widget gui::windows()
{
	return intf_->windows();
}

void gui::watch(Boolean)
{
}

void gui::clear()
{
	intf_->clear();
}


Pixel gui::pixel(const char* name)
{
	static str grey("grey");

	char buf[1024];

	sprintf(buf,"color_%s",name);

	str s = option<str>(globals::instance(),buf,grey);

	XrmValue from_value, to_value;

	from_value.addr = (char*)s.c_str();
	from_value.size = strlen(from_value.addr) + 1;

	Pixel p = 0;

	to_value.addr = (char*)&p;
	to_value.size = sizeof(Pixel);

	XtConvertAndStore(gui::top(),
		XmRString, &from_value,
		XmRPixel, &to_value);

	return p;
}


static GC makegc(Pixel p)
{
	XGCValues values;
	XtGCMask  valuemask  = GCForeground;
	values.foreground     = p;

	Widget w = gui::top();
	return XCreateGC(XtDisplay(w),XtWindow(w),valuemask,&values);
}

static XmFontList makefont(const char* name)
{
	XrmValue from_value, to_value;

	from_value.addr = (char*)name;
	from_value.size = strlen( from_value.addr ) + 1;

	XmFontList p = 0;

	to_value.addr = (char*)&p;
	to_value.size = sizeof(XmFontList);

	XtConvertAndStore(gui::top(),
		XmRString, &from_value,
		XmRFontList, &to_value);

	return p;
}

static XmFontList font(const char* name)
{
	char buf[1024];

	sprintf(buf,"%s_plain",name);
	str plain = option<str>(globals::instance(),buf,"fixed");

	sprintf(buf,"%s_bold",name);
	str bold  = option<str>(globals::instance(),buf,"fixed");

	str f = plain + str("=normal,") + bold + str("=bold");
	return makefont(f.c_str());
}

inline GC makegc(const char* p)
{
  return makegc(gui::pixel(p));
}

static Pixel *status_colors = 0;

char *ecf_colors_name[]
= { (char*)"unknown", (char*)"suspended", (char*)"complete", (char*)"queued", (char*)"submitted", (char*)"active",
    (char*)"aborted", (char*)"shutdown",  (char*)"halted"  ,  
    (char*)"meter_low",(char*)"threshold"  ,   (char*)"event"  ,  
    NULL };

Pixel gui::colors(unsigned int n)
{
	if(status_colors == 0)
	{
          status_colors = new Pixel[XtNumber(::ecf_colors_name)];
          for(unsigned int i = 0 ; i < XtNumber(::ecf_colors_name); i++)
            status_colors[i] = gui::pixel(::ecf_colors_name[i]);
	}
	return status_colors[n];
}

static XmFontList normalFont = 0;
static XmFontList smallFont  = 0;
static XmFontList tinyFont  = 0;

XmFontList gui::fontlist(void)
{
	if(normalFont == 0)
		normalFont = font("normal_font");
	return normalFont;
}

XmFontList gui::smallfont(void)
{
	if(smallFont == 0)
		smallFont = font("small_font");
	return smallFont;
}

XmFontList gui::tinyfont(void)
{
	if(tinyFont == 0)
		tinyFont = font("tiny_font");
	return tinyFont;
}

GC gui::blackGC(void)
{
	static GC gc = makegc("black");
	return gc;
}

GC gui::blueGC(void)
{
	static GC gc = makegc("blue");
	return gc;
}

GC gui::redGC(void)
{
	static GC gc = makegc("red");
	return gc;
}

static GC* status_gc = 0;
GC gui::colorGC(unsigned int n)
{
	if(status_gc == 0)
	{
          status_gc = new GC[XtNumber(::ecf_colors_name)];
          for(unsigned int i = 0 ; i < XtNumber(::ecf_colors_name); i++)
            status_gc[i] = makegc(colors(i));
	}
	if(n < 0 || n>= XtNumber(::ecf_colors_name)) return blackGC();
	return status_gc[n];
}


void gui::changed(resource& c)
{
	for(unsigned int i = 0; i < XtNumber(gui_resources) ; i++)
	{
		if(&c == gui_resources[i])
		{
			delete[] status_colors; status_colors = 0;
			delete[] status_gc;     status_gc     = 0;

			normalFont = 0;
			smallFont  = 0;

			host::redraw_all();
			break;
		}
	}
}

void gui::rename_host(const std::string& a,const std::string& b)
{
	intf_->rename_host(a,b);
}

void gui::remove_host(const std::string& a)
{
	intf_->remove_host(a);
}

void gui::set_interface(interface* i)
{
	intf_= i;
}

void gui::error(const char* fmt,...)
{
        char buf[10240];
	va_list arg;
	va_start(arg,fmt);

	vsprintf(buf,fmt,arg);
	va_end(arg);

	std::string msg = buf;
	split_msg(msg);
	intf_->error(msg.c_str());
}

void gui::error(ecf_list *l)
{    
  // char buf[10240];
    char buf[240];
    buf[0] = 0;
    while(l)
    {
      if(strlen(l->name().c_str()) + 2 >= sizeof(buf))
            break;
        if(buf[0]) strcat(buf,"\n");
        strcat(buf,l->name().c_str());
        l = l->next;
    }
	intf_->error(buf);
}

void gui::syserr(const char* msg)
{
  gui::error("%s",msg);
}

bool gui::visible()
{
	return intf_->visible();
}
