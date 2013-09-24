#ifndef option_panel_H
#define option_panel_H

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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

#include "uioption.h"

#ifndef panel_H
#include "panel.h"
#endif

#ifndef pref_editor_H
#include "pref_editor.h"
#endif

class option_panel : public panel, public option_form_c , public pref_editor{
public:

  option_panel(panel_window&);
  
  ~option_panel(); // Change to virtual if base class
  
  virtual const char* name() const { return "Options"; }
  virtual void show(node&);
  virtual void clear();
  virtual Boolean enabled(node&);
  
  virtual Widget widget() { return option_form_c::xd_rootwidget(); }
  
  virtual void create (Widget parent, char *widget_name = NULL);
  
private:

  option_panel(const option_panel&);
  option_panel& operator=(const option_panel&);
  
  virtual void changedCB( Widget w, XtPointer ) { pref_editor::changed(w); }
  virtual void useCB( Widget w, XtPointer )     { pref_editor::use(w);     }
  
  virtual Widget form()   { return form_; }
  virtual configurable* owner();  
};

inline void destroy(option_panel**) {}
#endif
