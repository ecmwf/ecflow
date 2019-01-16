//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #17 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include "variable_node.h"

#ifndef ecf_node_
#include "ecf_node.h"
#endif
struct ecf_variable {
	int type;
	char *name;
	struct ecf_variable *next;
	int status;
	int nid;
	int act_no;
	class ecf_node *parent;
	void* user_ptr;
	int user_int;
	class ecf_node *kids;
	char *value;
};
typedef struct ecf_variable ecf_variable;

const xmstring& variable_node::labelTree()
{
  labelTree_ = make_label_tree();
  return labelTree_;
}

#ifdef BRIDGE
variable_node::variable_node(host& h,sms_node* n, char b) 
  : node(h,n,b) 
  , generated_(b == 'g') 
{}
#endif

variable_node::variable_node(host& h,ecf_node* n) 
  : node(h,n)
  , generated_(false) 
{
  generated_ = n ? (n->kind() == 'g') : false;
}

std::string variable_node::get_var(bool substitute) {
#ifdef BRIDGE
  if (tree_) {
    ecf_variable *var = (ecf_variable*) tree_;
    if (var) return var->value;
  } else 
#endif
    if (parent()) 
      if (parent()->__node__())
        return parent()->__node__()->get_var(name(), 
                                             generated_, 
                                             substitute);
  return ecf_node::none();
}

xmstring variable_node::make_label_tree()
{
  const std::string& v = get_var();
  return xmstring(name().c_str()) + xmstring("=") + xmstring(v.c_str());
}

void variable_node::drawNode(Widget w,XRectangle *r,bool)
{
  XmStringDraw(XtDisplay(w),XtWindow(w),
	       smallfont(),
	       labelTree(),
	       generated_?blueGC():blackGC(),
	       r->x+2,
	       r->y+2,
	       r->width,
	       XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
  /* shadow(w,r); */
}

Boolean variable_node::visible() const 
{ 
  return 
    generated_ ?
    show::want(show::genvar) :
    show::want(show::variable);      
}

void variable_node::info(std::ostream&f)
{
  if (generated_)
    f << "  #  ( " << name() << "\t: " << get_var() + ")\n";
  else
    f << "  edit " << name() <<  "\t" <<  get_var() << "\n";
}

bool variable_node::match(const char* p)
{
  return (strstr(name().c_str(),p) != 0) 
    || (strstr(get_var().c_str(),p) != 0);
}

void variable_node::edit(node_editor& e)
{
  e.set("name", str(name()));
  e.set("value",str(get_var()));
}

void variable_node::apply(node_editor& e)
{
  str value;
  e.get("value",value);
  const char* name = this->name().c_str();
  const char* kind = "add"; 
  for (node* n = parent()->kids(); n; n = n->next())
    if (n->type() == NODE_VARIABLE && n->name() == name)
      { kind = "change"; break; }
  serv().command(clientName,"--alter", kind, "variable",
		 name, value.c_str(), parent()->full_name().c_str(),
		 NULL);
}

void variable_node::perlify(FILE* f)
{
  perl_member(f,"value",get_var().c_str());
}
