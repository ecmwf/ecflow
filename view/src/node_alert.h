#ifndef node_alert_H
#define node_alert_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#include "uinode_alert.h"
#include "window.h"
#include "singleton.h"
#include "node_list.h"
#include "gui.h"
#include "node.h"

class node;

template<class T>
class node_alert : public node_alert_shell_c, public window, 
  public singleton<T>, public node_list {

  const char* alert_;
  void notify_system(node* n);
 public:

  node_alert(const char*,int = -1);
  
  virtual ~node_alert(); // Change to virtual if base class
  
  virtual Widget shell() { return _xd_rootwidget; }
  virtual Widget list()  { return list_; }
  virtual Widget form()  { return form_; }
  
  // HP compiler wants the 'singleton<T>::' specifier :-(
  static void show()        
  { if(gui::visible()) { singleton<T>::instance().add(0);singleton<T>::instance().notify_system(0);}}
  
  static void show(node& n) 
  { if(gui::visible()) { singleton<T>::instance().add(&n);singleton<T>::instance().notify_system(&n); }}
  
  static void hide(node& n) 
  { if(gui::visible()) singleton<T>::instance().remove(&n); }

  static void clear()       
  { if(gui::visible()) singleton<T>::instance().reset();  }

private:

	node_alert(const node_alert<T>&);
	node_alert<T>& operator=(const node_alert<T>&);

	std::string title_;
	int bg_;

	void browseCB(Widget,XtPointer);
	void clearCloseCB( Widget, XtPointer ) ;
	void closeCB( Widget, XtPointer ) ;
	void collectCB( Widget, XtPointer ) ;
};


#if defined(__GNUC__) || defined(hpux) || defined(_AIX)
#include "node_alert.cc"
#endif

#endif
