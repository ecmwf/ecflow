#ifndef host_prefs_H
#define host_prefs_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#ifndef prefs_H
#include "prefs.h"
#endif

#ifndef uioption_H
#include "uioption.h"
#endif

class host_prefs : public prefs, public option_form_c {
public:
  host_prefs() {}
  
  ~host_prefs() {}
  
  virtual Widget widget() { return _xd_rootwidget; }

private:

  host_prefs(const host_prefs&);
  host_prefs& operator=(const host_prefs&);
  
  virtual void changedCB( Widget w, XtPointer ) { pref_editor::changed(w); }
  virtual void useCB( Widget w, XtPointer )     { pref_editor::use(w); }
  virtual void create(Widget w,char*);
};

inline void destroy(host_prefs**) {}
#endif
