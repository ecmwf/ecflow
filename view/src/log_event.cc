//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #10 $ 
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

#include <stdlib.h>
#include "log_event.h"
#include "ecflowview.h"
#include "gui.h"
#include "host.h"
#include "node.h"
#include "parser.h"
#include "array.h"
#include "ecf_node.h"

const int boxSize = 3;

namespace status {
const char *status_name[10]= { 
  (char*)"unknown", (char*)"suspended", (char*)"complete", (char*)"queued",
  (char*)"submitted", (char*)"active", (char*)"aborted", (char*)"shutdown",  
  (char*)"halted"  ,  NULL };
}

static event_sorter* sorter = 0;

class log_cache : public array<log_event*> {
public:
	void reset();
	void sort();
	~log_cache() { reset(); }
};

static log_cache  cache;
static str        cached;

void log_cache::reset()
{
	int c = count();
	for(int i = 0; i < c ; i++)
		(*this)[i]->detach();
	clear();
}

static int compare(const void* a,const void* b)
{
	log_event** ea = (log_event**)a;
	log_event** eb = (log_event**)b;
	return sorter->compare(*ea,*eb);
}

void log_cache::sort()
{
	qsort(values_,count_,sizeof(log_event*),compare);
}

static node* gn = 0;

log_event::log_event(node* n,const DateTime& time):
	time_(time),
	node_(n)
{
	attach();
	cache.add(this);
	observe(n);
}

log_event::~log_event()
{
}


void log_event::load(host& h,const char* name,bool reset)
{
  if(reset)
    {
      cache.reset();
      cached = str();
    }
  
  if(str(name) != cached)
    {
      gn = h.top();
      std::string varlog = gn->variable("ECF_LOG");
      std::string varhom = gn->variable("ECF_HOME");
      std::string varnod = gn->variable("ECF_NODE");   

      if (gn->variable("ECF_PORT") == ecf_node::none()) {
	varlog = gn->variable("SMSLOG");
	varhom = gn->variable("SMSHOME");
	varnod = gn->variable("SMSNODE");
      }
      char* vartmp = getenv("TMPDIR");
      
      char buf[1024];
      const char* p = name;
      
      while(*p)
	{
	  int i = 0;
	  while(*p && *p != ' ')
	    buf[i++] = *p++;
	  buf[i] = 0;
	  
	  if(i) {
	    struct stat st;
	    if (stat(buf, &st) == (-1)) {
	      if (vartmp) { 
		char cmd[1024];

		sprintf(cmd,"%s/%s",vartmp,varlog.c_str());
		if (stat(cmd, &st) == (-1)) {
		  sprintf(cmd,"rcp %s:%s/%s %s/.", varnod.c_str(), varhom.c_str(), varlog.c_str(), 
			  getenv((char*)"TMPDIR"));
		  printf("%s\n", cmd);
		  system(cmd);
		  
		  ::sleep(1);
		  sprintf(cmd,"%s/%s",vartmp,varlog.c_str());
		  { if (stat(cmd, &st) == (-1)) {
		      sprintf(cmd,"scp %s:%s/%s %s/.", varnod.c_str(), varhom.c_str(), varlog.c_str(), 
			      getenv((char*)"TMPDIR"));
		      printf("%s\n", cmd);
		      system(cmd);
		    }
		    sprintf(cmd,"%s/%s",vartmp,varlog.c_str());
		    ::sleep(1);
		  }

		  if (stat(cmd, &st) != (-1)) parser::parse(cmd);
		} else {
		  parser::parse(cmd);
		};
	      }
	    } else {
	      parser::parse(buf);
	    }
	  }
	  if(*p) p++;
	}
      
      gn = 0;
      cached = name;
	}  
}

void log_event::sort(event_sorter& l)
{
	sorter = &l;
	cache.sort();
	sorter = 0;
}

void log_event::scan(node* n,event_lister& l)
{
	int c = cache.count();
	for(int i = 0; i < c ; i++)
		if(cache[i]->node_ != 0)
			if(cache[i]->node_->is_my_parent(n))
				l.next(cache[i]);
}

const node* log_event::find(const char* name)
{
	return gn?gn->find(name):0;
}


void log_event::size(Widget,XRectangle* r)
{
	r->width = r->height = 2*boxSize;
}

void log_event::draw(Widget w,XRectangle* r)
{
	GC gc  = gui::blackGC();
	XFillRectangles(XtDisplay(w), XtWindow(w), gc, r, 1); // was comment
	XDrawLine(XtDisplay(w), XtWindow(w), gc,
		r->x,
		r->y,
		r->x + r->width,
		r->y + r->height);

	XDrawLine(XtDisplay(w), XtWindow(w), gc,
		r->x,
		r->y + r->height,
		r->x + r->width,
		r->y);
}

class status_event : public log_event {
	int status_;
	virtual void draw(Widget,XRectangle*);

	virtual bool start();
	virtual bool end();
	virtual char* text(char*);

	virtual int status() { return status_; }

public:
	status_event(node* n,const DateTime& time,int status):
		log_event(n,time), status_(status) {}
};

char* status_event::text(char* buf)
{
	sprintf(buf,"%s %s is %s",node_->type_name(),node_->full_name().c_str(),
		status::status_name[status_]);
	return buf;
}

class event_event : public log_event {
	bool set_;
	virtual node* owner() { return node_->parent(); }
	virtual void draw(Widget,XRectangle*);
	virtual char* text(char*);
public:
	event_event(node* n,const DateTime& time,bool s): log_event(n,time), set_(s) {}
};

char* event_event::text(char* buf)
{
	sprintf(buf,"event %s is %s",
		node_->full_name().c_str(),
		set_?"set":"cleared");
	return buf;
}

//===========================================
void event_event::draw(Widget w,XRectangle* r)
{
	if(set_)
		XFillRectangles(XtDisplay(w),XtWindow(w),
			gui::blueGC(),r,1);
	else
		XDrawRectangles(XtDisplay(w),XtWindow(w),
			gui::blueGC(),r,1);
}

//===========================================
class meter_event : public log_event {
	int step_;
	virtual node* owner() { return node_->parent(); }
	virtual void draw(Widget,XRectangle*); 
	virtual char* text(char*);
public:
	meter_event(node* n,const DateTime& time,int s): log_event(n,time), step_(s) {}
};

char* meter_event::text(char* buf)
{
  sprintf(buf,"meter %s reaches %d",node_->full_name().c_str(),step_);
  return buf;
}

void meter_event::draw(Widget w,XRectangle* r)
{
	XDrawArc(XtDisplay(w),
			XtWindow(w),
			gui::blackGC(),
			r->x,
			r->y,
			r->height,
			r->height,
			0,360*64);	
#if 0
	char buf[80];
	sprintf(buf,"%d",step_);
	xmstring s(buf);
    XmStringDraw(XtDisplay(w),XtWindow(w),
        gui::tinyfont(),
        s,
        gui::blackGC(),
        r->x,
        r->y,
        r->width,
        XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, r);
#endif
}

bool status_event::start() 
{ 
	return status_ == STATUS_SUBMITTED; 
}

bool status_event::end()   
{ 
	return status_ == STATUS_COMPLETE; 
}

//=========================================================

void status_event::draw(Widget w,XRectangle* r)
{
	GC gc        = gui::colorGC(status_);
#if 1
	XFillArc(XtDisplay(w),
			XtWindow(w),
			gc,
			r->x,
			r->y,
			r->height,
			r->height,
			0,360*64);	
#else
	XDrawLine(XtDisplay(w), XtWindow(w), gc,
		r->x,
		r->y,
		r->x + r->width,
		r->y + r->height);

	XDrawLine(XtDisplay(w), XtWindow(w), gc,
		r->x,
		r->y + r->height,
		r->x + r->width,
		r->y);
#endif
}

void log_event::status_event(const DateTime& t,node* n,int s)
{
  if (0) {
    if (n)
      std::cout << "# event: " << t.date << " " << t.time << " " << n->full_name() << " " << status::status_name[s] << "\n";
    else 
      std::cout << "# event: " << t.date << " " << t.time << " no path " << status::status_name[s] << "\n";
  }
  if(n)	new ::status_event(n,t,s);
}

void log_event::event_event(const DateTime& t,node* n,bool set)
{
  if(n) new ::event_event(n,t,set);
}

void log_event::meter_event(const DateTime& t,node* n,int s)
{
  if(n) new ::meter_event(n,t,s);
}

void log_event::gone(observable*)
{
  node_ = 0;
}
