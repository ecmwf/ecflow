#ifndef log_event_H
#define log_event_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#ifdef NO_BOOL
#include "bool.h"
#endif

#ifndef counted_H
#include "counted.h"
#endif

#ifndef observer_H
#include "observer.h"
#endif

#include <Xm/Xm.h>
#include "SimpleTime.h"

class node;
class host;
class log_event;
class str;

class event_lister {
public:
	virtual void next(log_event*) = 0;
};

class event_sorter {
public:
	virtual int compare(log_event*,log_event*) = 0;
};

inline
bool operator<(const DateTime& d1,const DateTime& d2)
{
	return d1.date < d2.date || (d1.date == d2.date && d1.time < d2.time);
}

inline
bool operator>(const DateTime& d1,const DateTime& d2)
{
	return d2 < d1;
}

inline
bool operator<=(const DateTime& d1,const DateTime& d2)
{
	return d1.date <= d2.date || (d1.date == d2.date && d1.time <= d2.time);
}


inline
bool operator==(const DateTime& d1,const DateTime& d2)
{
	return (d1.date == d2.date) && (d1.time == d2.time);
}

inline
bool operator!=(const DateTime& d1,const DateTime& d2)
{
	return !(d1==d2);
}

const DateTime kSmallDate = { 19000101, 0};
const DateTime kLargeDate = { 21000101, 0};

class log_event : public counted, public observer {
public:

	log_event(node*,const DateTime&);

	const DateTime& time() const { return time_; }

	virtual bool start() { return false; }
	virtual bool end() { return false; }

	virtual node* owner() { return node_; }
	virtual node* get_node() { return node_; }

	virtual int status() { return -1; }

	virtual char* text(char*)  = 0;

	virtual void draw(Widget,XRectangle*);
	virtual void size(Widget,XRectangle*);

	static void status_event(const DateTime&,node*,int);
	static void event_event(const DateTime&,node*,bool);
	static void meter_event(const DateTime&,node*,int);

	static void load(host&,const char*,bool = false);
	static void scan(node*,event_lister&);
	static void sort(event_sorter&);
	static const node* find(const char*);

	static int compare(const log_event*,const log_event*);

protected:

	virtual ~log_event(); // Change to virtual if base class

	DateTime   time_;
	node*      node_;

private:

	log_event(const log_event&);
	log_event& operator=(const log_event&);

	void notification(observable*)              {                   }
	void adoption(observable* o,observable* n)  { node_ = (node*)n; }
	void gone(observable*);
};


#endif
