//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #15 $ 
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
#include <boost/algorithm/string.hpp>    

#include "label_node.h"
#include "show.h"
#include "text_lister.h"

#ifndef ecf_node_
#include "ecf_node.h"
#endif

const Label& label_node::get() {
  ecf_concrete_node<const Label>* base = 
    dynamic_cast<ecf_concrete_node<const Label>*> (owner_);
  if (base) return *(base->get());
  if (parent() && parent()->__node__())
    return parent()->__node__()->get_label(name());
  return Label::EMPTY();
}

xmstring label_node::make_label_tree()
{  
  return xmstring(name().c_str(),"bold") + xmstring(": ","bold") 
    + xmstring(value());
}

void label_node::drawNode(Widget w,XRectangle* r,bool)
{
  std::string msg = value();
  boost::algorithm::to_lower(msg);
  bool red = std::string::npos != msg.find("error");

    XmStringDraw(XtDisplay(w),XtWindow(w),
        smallfont(),
        labelTree(),
        red ? redGC() : blackGC(),
        r->x+2,
        r->y+2,
        r->width,
        XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, r);
	shadow(w,r);
}

void label_node::info(std::ostream& f)
{
	node::info(f);
	f << "\nText:\n";
	f << "-----\n";
	f << value();
	f << "\n\nDefault:\n";
	f << "-------\n";
	f << def() << "\n";
}

void label_node::perlify(FILE* f)
{
  perl_member(f,"value",  value());
  perl_member(f,"default",def());
}

const char* label_node::value()
{
  // static bool prb = false; if (prb) return 0x0; prb = true; 
#ifdef BRIDGE
  if (tree_) return ((sms_label*) tree_)->value;
#endif
  const Label& lab = get();
  if (lab.new_value().empty() || lab.new_value() == "") 
    return def();
  return lab.new_value().c_str();
}

const char* label_node::def()
{
#ifdef BRIDGE
  if (tree_) return ((sms_label*) tree_)->def;
#endif
  return get().value().c_str();
}

Boolean label_node::visible() const { return show::want(show::label); }
