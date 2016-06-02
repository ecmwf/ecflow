#ifndef user_prefs_H
#define user_prefs_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#ifndef prefs_H
#include "prefs.h"
#endif

#ifndef uiuser
#include "uiuser.h"
#endif


class user_prefs : public prefs, public user_form_c {
public:

  user_prefs() {}
  
  ~user_prefs() {}
  
  virtual Widget widget() { return _xd_rootwidget; }

private:

  user_prefs(const user_prefs&);
  user_prefs& operator=(const user_prefs&);
  
  virtual void changedCB( Widget w, XtPointer ) { pref_editor::changed(w); }
  virtual void useCB( Widget w, XtPointer )     { pref_editor::use(w);     }
  
  virtual void create(Widget w,char*);
};

inline void destroy(user_prefs**) {}
#endif
