//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #17 $ 
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

#include "trigger_node.h"
#include "ExprAst.hpp"
// #include "text_lister.h"

trigger_node::trigger_node(host& h,ecf_node* n) 
  : node(h,n) 
  , expression_ ("empty")
  , full_name_ ("empty")
  , complete_ (false)
{
  if (!n) return;
  complete_ = (n->kind() == 'c'); 
  expression_ = n->toString();
  full_name_  = n->parent()->full_name();
  full_name_ += ":trigger";
}

const AstTop* trigger_node::get() const
{
  if (!owner_.get()) return 0x0;
  return dynamic_cast<ecf_concrete_node<ExpressionWrapper>*>
    (owner_.get())->get()->get_ast_top();
}

xmstring trigger_node::make_label_tree()
{
  int inc = 0;
  if (expression_.size() > 9) {
    inc = complete_ ? 9 : 8;
  }    
#ifdef BRIDGE
  if (tree_) inc = 0;
#endif
  return xmstring(expression_.c_str() + inc);
}

void trigger_node::drawNode(Widget w,XRectangle* r,bool tree)
{
  XmStringDraw(XtDisplay(w),XtWindow(w),
               smallfont(),
               tree?labelTree():labelTrigger(),
               complete_?blueGC():blackGC(),
               r->x+2,
               r->y+2,
               r->width,
               XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);

  shadow(w,r);
}

void trigger_node::info(std::ostream& f)
{
  const AstTop *ast = get();
  if (ast) {
    std::string str = ast->expression(true);
    f << str << "\n";
  }
}

// const std::string& trigger_node::definition() const {  return expression_; }

const std::string& trigger_node::name() const
{
  static std::string trigger_ = "trigger";
  return trigger_; 
}

void trigger_node::perlify(FILE* f)
{
  perl_member(f, "math", expression_.c_str());
}

#ifdef BRIDGE
#define K_NIL    0      
#define K_OR     1
#define K_AND    2
#define K_EQ     3
#define K_NE     4
#define K_LT     5
#define K_LE     6
#define K_GT     7
#define K_GE     8
#define K_ADD    9
#define K_SUB   10
#define K_MUL   11
#define K_DIV   12
#define K_MOD   13
#define K_POW   14
#define K_NOT   15     
#define K_UNARY 16     
#define K_OPEN  17     
#define K_CLOSE 18     
#define K_NAME  19     
#define K_PRECEDENCE 9             

static std::string buf;

inline void add(char* s,char* p,Boolean done)
{
  strcat(s,p);
}

static bool match_math(const Ast *m, sms_tree *t, const char* n)
{
  if (m) return m->evaluate();
  else if (t) {
    if (t->mtype == K_NAME) return strstr(t->name, n) != 0;
    return match_math(0, t->left, n) || match_math(0, t->right, n); 
  }
  return false;
}

bool trigger_node::match(const char* n)
{
  return match_math(get(),(sms_tree*)tree_, n);
}

static void print_math(char* s,sms_tree *m,Boolean done)
{
  if(!m) return;

  if(m->mtype == K_NAME)    {
    add(s,m->name,done);
    return;
  }

  switch(m->mtype)    {

   case K_NAME:
          add(s,m->name,done);
          break;

   case K_OR   :
      print_math(s,m->left,done);
      add(s,(char*)" or ",done);
      print_math(s,m->right,done);
      break;

   case K_AND  :
      print_math(s,m->left,done);
      add(s,(char*)" and ",done);
      print_math(s,m->right,done);
      break;

   case K_EQ   :
      print_math(s,m->left,done);
      add(s,(char*)" == ",done);
      print_math(s,m->right,done);
      break;

   case K_NE   :
      print_math(s,m->left,done);
      add(s,(char*)" != ",done);
      print_math(s,m->right,done);
      break;

   case K_LT   :
      print_math(s,m->left,done);
      add(s,(char*)" < ",done);
      print_math(s,m->right,done);
      break;

   case K_LE   :
      print_math(s,m->left,done);
      add(s,(char*)" <= ",done);
      print_math(s,m->right,done);
      break;

   case K_GT   :
      print_math(s,m->left,done);
      add(s,(char*)" > ",done);
      print_math(s,m->right,done);
      break;

   case K_GE   :
      print_math(s,m->left,done);
      add(s,(char*)" >= ",done);
      print_math(s,m->right,done);
      break;

   case K_ADD  :
      print_math(s,m->left,done);
      add(s,(char*)" + ",done);
      print_math(s,m->right,done);
      break;

   case K_SUB  :
      print_math(s,m->left,done);
      add(s,(char*)" - ",done);
      print_math(s,m->right,done);
      break;

   case K_MUL  :
      print_math(s,m->left,done);
      add(s,(char*)" * ",done);
      print_math(s,m->right,done);
      break;

   case K_DIV  :
      print_math(s,m->left,done);
      add(s,(char*)" / ",done);
      print_math(s,m->right,done);
      break;

   case K_MOD  :
      print_math(s,m->left,done);
      add(s,(char*)" % ",done);
      print_math(s,m->right,done);
      break;

   case K_POW  :
      print_math(s,m->left,done);
      add(s,(char*)" ** ",done);
      print_math(s,m->right,done);
      break;

   case K_NOT  :
     add(s,(char*)"not ",done);
      print_math(s,m->right,done);
      break;

   case K_UNARY:
     add(s,(char*)"- ",done);
      print_math(s,m->right,done);
      break;

   case K_OPEN :
     add(s,(char*)"(",done);
      print_math(s,m->right,done);
      add(s,(char*)")",done);
      break;

   default :
     add(s,(char*)"--\?\?\?--",done);
      break;
    }
}

extern "C" {
#define new _new
#define delete _delete
#include "smsproto.h"
}

trigger_node::trigger_node(host& h,sms_node* n, char b) 
  : node(h,n,b) 
  , expression_ ()
  , complete_ (b == 'c')
{
  char buf[10240] = { 0 }; 
  sms_trigger* trg = (sms_trigger*) n;
  if (!trg) return;
  tree_ = (sms_node*) trg->math;
  print_math(buf, (sms_tree*) trg->math, False);
  expression_ = buf;
  full_name_  = sms_node_full_name(n->parent);
  full_name_ += ":trigger";
}
#endif
