//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "text_window.h"
#include <string>
#include "viewer.h"
#include "fsb.h"
#include "ask.h"
#include "globals.h"
#include <Xm/Text.h>
extern "C" {
#include "xec.h"
}

text_window::text_window(bool map_it):
	file_(0,false),
	text_map_(0),
	map_it_(map_it)
{
#if 1
	map_it_ = 0;
#endif
}


text_window::~text_window()
{
	xec_UnmapText(text_map_);
	text_map_ = 0;
}

void text_window::clear()
{
	find::hide();
	xec_UnmapText(text_map_);
	text_map_ = 0;
	XmTextSetString(text(),"");
	file_ = tmp_file(0,false);
}


void text_window::load(const tmp_file& x)
{
	file_ = x;
	xec_UnmapText(text_map_); text_map_ = 0;
	const int MAXLEN = 512;
        char cmsg[MAXLEN];
	snprintf(cmsg, MAXLEN, "Could not load file %s", file_.c_str());

	if(file_.c_str())
	{
		int zeros = 0;
		if(map_it_) {
			text_map_ = xec_MapText(text(),file_.c_str(),&zeros);
			if(text_map_ == 0)
				XmTextSetString(text(),cmsg);
			if(zeros) {
				find::make(text());
				find::message("This file contains %d null character%s.\n"
					"The find will not work properly.",
					zeros,
					zeros>1?"s":""
					);
			}
			else {
				find::no_message();
			}
		}
		else {
			int err = xec_LoadText(text(),file_.c_str(),False);
			if(err)
			  XmTextSetString(text(),cmsg);
		}
	}
	else
	  XmTextSetString(text(),cmsg);
}

class text_viewer : public viewer {
  tmp_file tmp_;
public:
     text_viewer(const tmp_file& t);
};

text_viewer::text_viewer(const tmp_file& t)
 : tmp_(t)
{
  if (!tmp_.c_str()) return;
  char buf[1024];
  const char* xpager = getenv("XPAGER");
  if (xpager)
    sprintf(buf,"${XPAGER:=xterm -e more} %s",tmp_.c_str());
  else
    sprintf(buf,"xterm -e ${PAGER:=more} %s",tmp_.c_str());
  FILE *f = popen(buf,"r");
  if(!f) {
    std::cerr << "# error: " << buf << "\n";
    return;
  }
  start(f);
}

void text_window::open_viewer()
{
  new text_viewer(file_);
}


void text_window::search(const char* p,bool case_sens,bool regex,
	bool back,bool wrap)
{

	bool  found = xec_TextSearch(text(),(char*)p,!case_sens,regex,back,
		false,wrap);

	if(!found)
		message("Text not found");
	else
		no_message();
}


void text_window::open_search()
{
	find::raise(text());
}


//-------------------------------------------------------

class text_saver : public viewer {
     tmp_file tmp_;
public:
     text_saver(const tmp_file& t);
};

text_saver::text_saver(const tmp_file& t):
   tmp_(t)
{
   const char* p = fsb::ask("Save as:");
   if(p)
   {
	   char buf[2048];
	   sprintf(buf,"cp %s %s 2>&1",tmp_.c_str(),p);
	   show(buf);
   }
   else delete this;
}

void text_window::save()
{
 	new text_saver(file_);
}

//-------------------------------------------------------

class text_printer : public viewer {
     tmp_file tmp_;
public:
     text_printer(const tmp_file& t);
};

text_printer::text_printer(const tmp_file& t):
   tmp_(t)
{
   static option<str> cmd(globals::instance(),"print_command","lpr");
	
   str c = cmd;

   if(ask::show(c,"Print command:"))
   {
	   cmd = c;
	   char buf[2048];
	   sprintf(buf,"%s %s 2>&1",c.c_str(),tmp_.c_str());
	   show(buf);
   }
}

void text_window::print()
{
 	new text_printer(file_);
}
