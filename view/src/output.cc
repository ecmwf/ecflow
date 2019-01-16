//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #18 $ 
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
#include "output.h"
#include "node.h"
#include "host.h"
#include <Xm/Text.h>
#include <Xm/TextStrSoP.h>
#include <Xm/List.h>
#include "ecf_node.h"
extern "C" {
#include "xec.h"
}

#include <sys/types.h>
#include <sys/stat.h>

output::output(panel_window& w):
	text_window(true),
	panel(w),
	file_(0x0)
{
}

output::~output()
{
	if(file_)
		free(file_);
}

void output::create (Widget parent, char *widget_name )
{
	output_form_c::create(parent,widget_name);
}

void output::clear()
{
	if(file_) free(file_);
	file_ = 0x0;
	XmTextSetString(name_,(char*) "");
	XmListDeleteAllItems(list_);
	//active(False);
	text_window::clear();
}

class output_lister : public lister<ecf_dir> {
	Widget list_;
	bool sort() { return true; }
	bool compare(ecf_dir&,ecf_dir&);
	void next(ecf_dir&);
public:
	output_lister(Widget l) : list_(l) {}
};

void output_lister::next(ecf_dir& d)
{
  if(S_ISREG(d.mode))
    {
      time_t t   = d.mtime;	
      time_t now = time(0);
      
      int delta  = now - t;
      if(delta<0) delta = 0;

      char buf[80];
      strcpy(buf,"Right now");
      
      if(delta >=1  && delta < 60)
	{
	  sprintf(buf,"%d second%s ago",delta,delta>1?"s":"");
	}
      
      if(delta >= 60 && delta < 60*60)
	{
	  sprintf(buf,"%d minute%s ago",delta/60,delta/60>1?"s":"");
	}
      
      if(delta >= 60*60 && delta < 60*60*24)
	{
	  sprintf(buf,"%d hour%s ago",delta/60/60,delta/60/60>1?"s":"");
	}
      
      if(delta >= 60*60*24)
	{
	  sprintf(buf,"%d day%s ago",delta/60/60/24,delta/60/60/24>1?"s":"");
	}
      
      xec_VaAddListItem(list_,(char*) "%-60s (%s)",d.name_,buf);
    }
}

bool output_lister::compare(ecf_dir& a,ecf_dir& b)
{
  return a.mtime > b.mtime;
}

class search_me : public runnable {
	find& find_;

	void run() {
	  /* text case regexp back wrap */
	  find_.search("System Billing Units",true,false,false,true);
	  find_.search("smscomplete",true,false,false,true);
	  find_.search("smsabort",true,false,false,true);
	  // display init but not appreciated when updating for tail:
	  // find_.search("ecflow_client",true,false,false,true); 
	  find_.search("xcomplete",true,false,false,true);
	  find_.search("xabort",true,false,false,true);
	  find_.search("--complete",true,false,false,true);
	  find_.search("--abort",true,false,false,true);
	  find_.no_message();
	  find_.pending(0);
	  delete this;
	}

public:
	search_me(find& f)  : find_(f) { find_.pending(this); enable();}
};

void output::show(node& n)
{
  std::string jobout = n.variable("ECF_JOBOUT");
  if (!n.__node__()) 
    jobout = n.variable("SMSJOBOUT");
  else if (!n.__node__()) return;
  else if (!n.__node__()->get_node()) return;
  else n.__node__()->get_node()->variableSubsitution(jobout);

  if(jobout == ecf_node::none()) {     
    clear();        
    return;    
  }

  /* output variable may contain micro */

  if(file_) free(file_);
  file_ = strdup(jobout.c_str());
  load(n);
  XmListDeleteAllItems(list_);
  
  output_lister ol(list_);
  n.serv().dir(n,file_,ol);
  
  std::string remote = n.variable("ECF_OUT");
  std::string job    = n.variable("ECF_JOB");
  if (!n.__node__()) { 
    remote = n.variable("SMSOUT"); 
    job    = n.variable("SMSJOB"); 
  }
  if (!remote.empty() && !job.empty()) {
    /* display both remote and local dir */
    if (remote == job) {
      output_lister rem(list_);
      n.serv().dir(n,job.c_str(),rem);
    }
  }
  new search_me(*this);  
}

struct dup_slash { // INT-74
  bool operator() (char x, char y) const {
    return x=='/' && y=='/';
  };
};

void output::load(node& n)
{
  if(file_) {
    std::string name (file_);
    name.erase(std::unique(name.begin(), name.end(), dup_slash()), name.end()); // INT-74
    XmTextSetString(name_,(char*)name.c_str()); /* ??? */
    tmp_file f = n.serv().file(n,name);
    text_window::load(f);
  } else {	  
    clear();
    tmp_file f = n.serv().output(n);
    text_window::load(f);
  }
}

void output::updateCB(Widget,XtPointer data)
{
	if(get_node())
		show(*get_node());
	else
		clear();
	XmTextShowPosition(text_,XmTextGetLastPosition(text_));
}

void output::browseCB(Widget,XtPointer data)
{
	XmListCallbackStruct *cb = (XmListCallbackStruct *) data;
	if(file_) free(file_);

	char *p = xec_GetString(cb->item);
	char buf[1024];
	sscanf(p,"%s",buf);
	XtFree(p);

	file_ = strdup(buf);

	if(get_node())
		load(*get_node());
	else
		clear();
}

Boolean output::enabled(node& n)
{
  if (n.type() != NODE_TASK && n.type() != NODE_ALIAS) return False;
  if (!n.__node__()) 
    return n.variable("SMSJOBOUT") != ecf_node::none();
  return n.variable("ECF_JOBOUT") != ecf_node::none();
}
