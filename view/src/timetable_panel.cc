//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <stdlib.h>
#include "arch.h"
#include "timetable_panel.h"
#include "node.h"
#include "host.h"
#include "fsb.h"
#include "gui.h"
#include "xnode.h"
#include "xmstring.h"
#include "globals.h"
#include "tmp_file.h"
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#include "log_event.h"
extern "C" {
#include "SimpleTime.h"
#include "Hyper.h"
}

namespace timetatble_status {
const char *status_name[10]
  = { "unknown", "suspended", "complete", "queued", "submitted", "active",
      "aborted", "shutdown",  "halted"  ,  NULL };
}

extern "C" {
#include "xec.h"
}
static void date2str(char* buf,const DateTime& dt)
{
	int ddd = dt.date;
	int ttt = dt.time;

	int yy = ddd / 10000; ddd %= 10000;
	int mm = ddd / 100; ddd %= 100;
	int dd = ddd;
	int HH = ttt / 10000; ttt %= 10000;
	int MM = ttt / 100; ttt %= 100;
        int ss = ttt ;
	sprintf(buf,"%04d-%02d-%02d %02d:%02d:%02d", 
		yy,mm,dd,HH,MM,ss);
}


static int sec2str(char* buf,int s)
{
	buf[0] = 0;
	if(s) {
			
		long x = s>0?s:-s;
		long n;
		char sec[20];
		char min[20];
		char hou[20];
		char day[20];

		*sec = *min = *hou = *day = 0;
		if((n = x % 60)) sprintf(sec,"%ld sec ",n);
		x /= 60;
		if((n = x % 60)) sprintf(min,"%ld min ",n);
		x /= 60;
		if((n = x % 24)) sprintf(hou,"%ld hour ",n);
		x /= 24;
		if((n = x)) sprintf(day,"%ld day ",n);

		sprintf(buf,"%s%s%s%s", day,hou,min,sec);
	}
	return s;
}

static int range2str(char* buf,const DateTime& dt1,const DateTime& dt2)
{
	return sec2str(buf,TimeDiff(dt1,dt2));
}

static void date2widget(Widget w,DateTime& x)
{
	char buf[80];
	if(x == kSmallDate)
		strcpy(buf,"-infinite");
	else if(x == kLargeDate)
		strcpy(buf,"+infinite");
	else
	{
		TimeAdd(&x,0);
		date2str(buf,x);
	}

	XmTextSetString(w,buf);
}

class timetable_node : public xnode {
#if 1
	// important: and alos in log_event
	void notification(observable*)        { redraw(); }
	void adoption(observable* o,observable* n) { node_ = (node*)n; }
	void gone(observable*)                { owner_.remove(this); delete this; }
#endif
protected:
	timetable_panel& owner_;
	log_event*       event_;
public:
	timetable_node(Widget w,timetable_panel& t,log_event* e);
	virtual ~timetable_node();
	log_event* event() { return event_; }
    	virtual DateTime date() { return event_->time();  }
	virtual char* text(char* b) { return event_->text(b); }
	virtual bool is_name() { return false; }
	virtual bool change_fold() {  return false; }
};

timetable_node::timetable_node(Widget w,timetable_panel& o,log_event* e)
  : xnode(e->get_node()), owner_(o), event_(e) 
{ 
  event_->attach();
}

timetable_node::~timetable_node()
{
  event_->detach();
}

//====================================================================

class time_event_node : public timetable_node {
protected:
  void draw(Widget w,XRectangle* r) { event_->draw(w,r); }
  void size(Widget w,XRectangle* r) { event_->size(w,r); }
  
public:
  time_event_node(Widget w,timetable_panel&,log_event*);
  ~time_event_node() {}
};

time_event_node::time_event_node(Widget w,timetable_panel& o,log_event* e)
  : timetable_node(w,o,e)
{
  TimeSetTime(w,getBox(w),e->time());
}

//====================================================================

class time_name_node : public timetable_node {
  bool fold_;
  void draw(Widget w,XRectangle* r);
  void size(Widget w,XRectangle* r);
public:
  time_name_node(Widget w,timetable_panel&,log_event*);
  ~time_name_node() {}
  virtual char* text(char* b) { 
    strcpy(b,node_->full_name().c_str());return b; }
  virtual bool is_name() { return true; }
  virtual bool change_fold() {  fold_ = !fold_; redraw(); return fold_; }
};

time_name_node::time_name_node(Widget w,timetable_panel& o,log_event* e)
  : timetable_node(w,o,e)
  , fold_(false)
{ 
  node_ = e->owner();
  getBox(w);
}

void time_name_node::draw(Widget w,XRectangle* r)
{
  xmstring s(node_->full_name().c_str(),fold_?"bold":"normal");
  XmFontList f = gui::tinyfont();
  XmStringDraw(XtDisplay(w),XtWindow(w),
        f,
        s,
        gui::blackGC(),
        r->x,
        r->y,
        r->width,
        XmALIGNMENT_CENTER, XmSTRING_DIRECTION_L_TO_R, r);
}

void time_name_node::size(Widget w,XRectangle* r)
{
        xmstring s(node_->full_name().c_str(),fold_?"bold":"normal");

	XmFontList f = gui::tinyfont();
	r->width    = XmStringWidth(f,s);
	r->height   = XmStringHeight(f,s);
}


//====================================================================

timetable_panel::timetable_panel(panel_window& w):
	panel(w),
	last_(kSmallDate),
	min_time_(kSmallDate),
	max_time_(kLargeDate)
{
	sorted_by_time_ = globals::get_resource("timeline_sorted_by_time",false);
	tasks_only_     = globals::get_resource("timeline_tasks_only",false);
}

timetable_panel::~timetable_panel()
{
	 clear();
}

void timetable_panel::create(Widget parent, char *widget_name )
{
	timetable_form_c::create(parent,widget_name);

	add_input_CB();

	date2widget(from_,min_time_);
	date2widget(to_,max_time_);

	XmToggleButtonSetState(by_time_,sorted_by_time_,False);
	XmToggleButtonSetState(by_name_,!sorted_by_time_,False);

	XmToggleButtonSetState(all_,!tasks_only_,False);
	XmToggleButtonSetState(tasks_,tasks_only_,False);
}

void timetable_panel::clear()
{
	NodeReset(time_);
	for(int i = 0; i < nodes_.count(); i++) 
		delete nodes_[i];
	nodes_.clear();
	XmTextSetString(file_,"");
 	last_ = kSmallDate;
	depend::hide();
}

void timetable_panel::show(node& n)
{
	clear();
	reload(false);
}

void timetable_panel::reload(bool reset)
{
  if(get_node())
    load(get_node()->serv().timefile().c_str(),reset);
  else 
    clear();
}

void timetable_panel::changed(node& n)
{
}

void timetable_panel::remove(timetable_node *t)
{
	nodes_.remove(t);
}

void timetable_panel::next(log_event* n)
{
	if(n->time() < min_time_ || n->time() > max_time_)
		return;

	if(tasks_only_ && n->owner()->type() != NODE_TASK) return;

	time_event_node *t = new time_event_node(time_,*this,n);
	int c = nodes_.count();
	bool found = false;

	for(int i = c-1 ; i >= 0; i--)
		if(nodes_[i]->event()->owner() == n->owner())
		{
			nodes_[i]->relation(t);
			found = true;
			break;
		}

	if(!found) {
		time_name_node *x = new time_name_node(time_,*this,n);
		nodes_.add(x);
		x->relation(t);
		x->visibility(true);
	}

	t->visibility(true);
	nodes_.add(t);
}

Boolean timetable_panel::enabled(node& n)
{
	return TRUE; 
}

struct sort_by_name : public event_sorter {
public:
	int compare(log_event* a,log_event* b) {
		char sa[1024];
		char sb[1024];

		node *nodea = a->get_node();
		node *nodeb = b->get_node();

		const char *ca = nodea ? nodea->full_name().c_str() : "none";
		const char *cb = nodeb ? nodeb->full_name().c_str() : "none";

		strcpy(sa,ca);
		strcpy(sb,cb);
		return strcmp(sa,sb);
	}
};

struct sort_by_time : public event_sorter {
public:
	int compare(log_event* a,log_event* b) {
		return TimeDiff(a->time(),b->time());
	}
};


static DateTime widget2date(Widget w)
{
	char *p = XmTextGetString(w);
	char buf[80];
	char *q = p;
	int i = 0;
	int j = 0;

	while(*q)
	{
		if(*q >= '0' && *q <= '9') buf[i++] = *q;
		if(*q == ' ') { buf[i++] = 0; j = i; }
		q++;
	}

	buf[i] = 0;
	XtFree(p);

	DateTime dt;
	dt.date = atol(buf);
	dt.time = atol(buf+j);

	TimeAdd(&dt,0);

	return dt;
}

void timetable_panel::load(const char* fname,bool reset)
{
	clear();
	if(get_node())
	{
		str path = fname;

		if(!reset && path != get_node()->serv().timefile()) 
			path = path + str(" ") + str(fname);

		get_node()->serv().timefile(path);

		min_time_ = widget2date(from_);
		max_time_ = widget2date(to_);

		if(min_time_ < kSmallDate) min_time_ = kSmallDate;
		if(max_time_ < kSmallDate) max_time_ = kLargeDate;

		date2widget(from_,min_time_);
		date2widget(to_,max_time_);


		XmTextSetString(file_,(char*)path.c_str());
		log_event::load(get_node()->serv(),path.c_str(),reset);


		if(sorted_by_time_)
		{
			sort_by_time sbt;
			log_event::sort(sbt);
		}
		else
		{
			sort_by_name sbn;
			log_event::sort(sbn);
		}

		log_event::scan(get_node(),*this);

	}
}

void timetable_panel::load(bool reset)
{
	const char* p = 0;
	if((p = fsb::ask("Load a timefile")))
		load(p,reset);
}

void timetable_panel::chooseCB( Widget, XtPointer data)
{
	load(true);
}

void timetable_panel::loadCB( Widget, XtPointer data)
{
	load(true);
}

void timetable_panel::mergeCB( Widget, XtPointer data)
{
	load(false);
}

void timetable_panel::activateCB(Widget w, XtPointer data)
{
	char* p = XmTextGetString(file_);
	load(p, w == file_);
	str s(p);
	XtFree(p);
}


void timetable_panel::updateCB( Widget, XtPointer data)
{
	reload(true);
}


void timetable_panel::hyperCB(Widget w, XtPointer data)
{
	panel::hyper(w,data);
}

static int compare(const void* a,const void *b)
{
	timetable_node** ta = (timetable_node**)a;
	timetable_node** tb = (timetable_node**)b;

	return TimeDiff( (*ta)->event()->time(), (*tb)->event()->time());
}

void timetable_panel::raw_click1(XEvent* e,xnode* x)
{
	char buf[1024];

	if(x == 0) x = (xnode*)TimeFindByY(time_,e); 

	timetable_node* t = main((timetable_node*)x);
	if(!t) return;


	node *m = t->get_node();

	tmp_file tmp(tmpnam(0));
	FILE *f = fopen(tmp.c_str(),"w");
	if(!f) return;

	range(t,dt1_,dt2_);

	if (m) fprintf(f,"{%s}",m->full_name().c_str());
	if(range2str(buf,dt1_,dt2_))
		fprintf(f," total time: %s",buf);
	fprintf(f,"\n\n");

	timetable_node** nodes = new timetable_node*[nodes_.count()];
	int count = 0;
        int i;

	for(i = 0; i < nodes_.count(); i++)
		if(!nodes_[i]->is_name() && nodes_[i]->event()->owner() == m)
			nodes[count++] = nodes_[i];

	qsort(nodes,count,sizeof(nodes[0]),compare);


	bool ok = false;
	int elapsed[STATUS_MAX];
	for(i = 0; i < STATUS_MAX; i++)
		elapsed[i] = 0;

	int prev_i = -1;

	for(i = 0; i < count; i++)
	{
		date2str(buf,nodes[i]->event()->time());
		fprintf(f,"%s",buf);
		fprintf(f," %s",nodes[i]->text(buf));

		if(i)
		{
			if(range2str(buf,
				nodes[i-1]->event()->time(),
				nodes[i]->event()->time()))
					fprintf(f," (%slater)",buf);

		}

		if(prev_i >= 0 && nodes[i]->event()->status() >= 0) 
		{
			elapsed[nodes[prev_i]->event()->status()] += 
				TimeDiff(nodes[i]->event()->time(), 
					 nodes[prev_i]->event()->time());
			ok = true;
		}

		int s = nodes[i]->event()->status();
		if(s >= 0) prev_i = i;

		fprintf(f,"\n");
	}	

	delete[] nodes;

	if(ok) {
		fprintf(f,"\nSummary:");
		fprintf(f,"\n--------\n");

		for(i = 0; i < STATUS_MAX; i++)
		  if(elapsed[i])
		    {
		      sec2str(buf,elapsed[i]);
		      fprintf(f,"%-10s: %s\n",timetatble_status::status_name[i],buf);
		    }
	}

	fclose(f);
	depend::make(widget());
	HyperLoadFile(hyper_,(char*)tmp.c_str());
	depend::raise(widget());
}

void timetable_panel::setToCB(Widget,XtPointer)
{
	date2widget(to_,dt2_);
	reload(false);
}

void timetable_panel::setFromCB(Widget,XtPointer)
{
	date2widget(from_,dt1_);
	reload(false);
}

void timetable_panel::setBothCB(Widget,XtPointer)
{
	date2widget(from_,dt1_);
	date2widget(to_,dt2_);
	reload(false);
}

void timetable_panel::resetCB(Widget w,XtPointer d)
{
	dt1_ = kSmallDate; dt2_ = kLargeDate;
	setBothCB(w,d);
}

void timetable_panel::raw_click2(XEvent* e,xnode* x)
{
	timetable_node *t = (timetable_node*)x;

	if(t && t->is_name())
	{
		node *m = t->get_node();
		bool x = !t->change_fold();

		for(int i = 0; i < nodes_.count(); i++)
			if(nodes_[i]->get_node() != m)
			  if(nodes_[i]->get_node())
			    if(nodes_[i]->get_node()->is_my_parent(m))
			      nodes_[i]->visibility(x);
		NodeUpdate(time_);
	}
	else {
	}
}

timetable_node* timetable_panel::main(timetable_node* t)
{
	if(!t) return 0;

	node *m = t->get_node();
	for(int i = 0; i < nodes_.count(); i++)
		if(nodes_[i]->is_name() && nodes_[i]->get_node() == m)
			return nodes_[i];
	return 0;
}



void timetable_panel::range(timetable_node* z,DateTime& dt1,DateTime& dt2)
{	
	dt1 = dt2 = z->event()->time();

	if(z->is_name())
	{
		node *m = z->get_node();

		for(int i = 0; i < nodes_.count(); i++)
			if(!nodes_[i]->is_name() && nodes_[i]->get_node() == m)
			{
				DateTime t = nodes_[i]->event()->time();
				if(t < dt1) dt1 = t;
				if(t > dt2) dt2 = t;
			}
	}
}
	
void timetable_panel::raw_click3(XEvent* e,xnode* x)
{
	char buf[1024];
	timetable_node* z = (timetable_node*)x;
	xmstring s("-");
	static xmstring cr("\n");

	XtUnmanageChild(set_both_);
	XtManageChild(set_to_);
	XtManageChild(set_from_);

	if(z) {

		z->text(buf);
		range(z,dt1_,dt2_);

		s = xmstring(buf);

		if(z->is_name())
		{
			date2str(buf,dt1_);
			s += cr;
			s += xmstring("From   : ","bold");
			s += xmstring(buf);

			date2str(buf,dt2_);
			s += cr;
			s += xmstring("To     : ","bold");
			s += xmstring(buf);

			if(range2str(buf,dt1_,dt2_))
			{
				s += cr;
				s += xmstring("Elapsed: ","bold");
				s += xmstring(buf);
			}

			XtManageChild(set_both_);
			XtUnmanageChild(set_to_);
			XtUnmanageChild(set_from_);

		}
		else {

			date2str(buf,dt1_);
			s = xmstring(buf) + cr + s;

			if(last_ != kSmallDate)
			{
				if(range2str(buf,last_,dt1_))
				{
					s += cr;
					s += xmstring("From last click: ","bold");
					s += xmstring(buf);
				}
			}

			last_ = dt1_;

		}
	}
	else {
		TimeEventTime(time_,e,&dt1_);
		date2str(buf,dt1_);
		s = xmstring(buf);
		dt2_  = dt1_;
		last_ = kSmallDate;
	}

	XtVaSetValues(label_, XmNlabelString, XmString(s), NULL);

	node_window::raw_click3(e,0);		
}


Widget timetable_panel::menu1()
{
	return menu_;
}

Widget timetable_panel::menu2()
{
    return menu1();
}

xnode* timetable_panel::xnode_of(node& n)
{
#if 0
    for(int i = 0; i < nodes_.count(); i++)
        if(nodes_[i]->get_node() == &n)
            return nodes_[i];
#endif
    return 0;
}

void timetable_panel::optionsCB(Widget,XtPointer)
{
	globals::set_resource("timeline_sorted_by_time",
		sorted_by_time_ = XmToggleButtonGetState(by_time_));

	globals::set_resource("timeline_tasks_only",
		tasks_only_ = XmToggleButtonGetState(tasks_));

	reload(false);
}
