#ifndef panel_H
#define panel_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#include "ecflowview.h"
#include "runnable.h"
#include "observer.h"

class node;
class panel_window;

class panel : public runnable, public observer  {
public:

	panel(panel_window&);

	virtual ~panel(); // Change to virtual if base class

	virtual void update();
	virtual void detach();
	virtual void freeze();
	virtual void submit();
	virtual void post_update();

	virtual void copy(panel*) {}

	virtual void  clear()        = 0;
	virtual void  show(node&)    = 0;
	virtual void  changed(node& n) { show(n); }
	virtual Boolean enabled(node& n) { return False; }

	virtual const char* name() const { return "(none)"; };
	virtual Widget widget()          = 0;
	virtual Widget menus(Widget) { return 0; }
	virtual Widget tools()       { return 0; } 


	virtual void print() {}
	virtual void save()  {}

	virtual bool can_print() { return false; }
	virtual bool can_save() { return false; }

protected:

	node* get_node() { return node_; }	
	void hyper(Widget,XtPointer, node* = 0);

private:

	panel(const panel&);
	panel& operator=(const panel&);

	panel* next_;
	node* node_;
	panel_window& owner_;

	void run();
	void notification(observable*)           { post_update(); }
	void gone(observable*)                   { post_update(); }
	void adoption(observable*,observable*)  { post_update(); }

	friend class panel_factory;
	friend class panel_window;
};

inline void destroy(panel**) {}

#ifndef panel_factories_H
#include "panel_factories.h"
#endif

class panel_factory {
  static panel_factory* factories_[PANEL_MAX_FACTORIES];
public:
  panel_factory(int);
  virtual panel* create(panel_window&,Widget) = 0;
  static panel* create_all(panel_window&,Widget);
};

template<class T>
class panel_maker : public panel_factory {
public:
	panel_maker(int n) : panel_factory(n) {}
	virtual panel* create(panel_window&,Widget);
};

template<class T>
panel* panel_maker<T>::create(panel_window& w,Widget parent)
{
	T* p = new T(w);
	p->create(parent,(char*)p->name());
	return p;
}

#endif
