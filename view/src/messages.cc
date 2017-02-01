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

#include "messages.h"
#include "node.h"
#include "host.h"
#include "selection.h"
#include "tmp_file.h"
#include "Hyper.h"
#include <stdio.h>
extern "C" {
#include "xec.h"
}

messages::messages(panel_window& w)
  :panel(w)
  ,text_window(false)
{
}

messages::~messages()
{
}

void messages::create (Widget parent, char *widget_name )
{
  messages_form_c::create(parent,widget_name);
}


void messages::clear()
{
  text_window::clear();
}

struct ml : public lister<ecf_list> {
  FILE* f_;
  void next(ecf_list& l) { if (f_) fprintf(f_,"%s\n",l.name().c_str()); }
public:
  ml(FILE* f): f_(f) {}
};

void messages::show(node& n)
{
  tmp_file tmp(tmpnam(0));
  FILE *f = fopen(tmp.c_str(),"w");
  if(!f) 	{ return; }
  
  const std::vector<std::string>& l = n.messages();
  std::vector<std::string>::const_iterator it = l.begin();
  
  while (it != l.end()) {
    fprintf(f, "%s\n", it->c_str());
    ++it;
  }
  fclose(f);
  
  load(tmp);
}

Boolean messages::enabled(node& n)
{
  return n.hasMessages();
}

// static panel_maker<messages> maker(PANEL_MESSAGES);
