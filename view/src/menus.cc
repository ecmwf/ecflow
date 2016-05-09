//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #17 $ 
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

#ifndef menus_H
#include "menus.h"
#endif

#ifndef selection_H
#include "selection.h"
#endif

#ifndef flags_H
#include "flags.h"
#endif

#ifndef tip_H
#include "tip.h"
#endif

#ifndef xec_H
#include "xec.h"
#endif

#ifndef collector_H
#include "collector.h"
#endif

#ifndef confirm_H
#include "confirm.h"
#endif

#ifndef panel_window_H
#include "panel_window.h"
#endif

#ifndef parser_H
#include "parser.h"
#endif

#ifndef directory_H
#include "directory.h"
#endif

#include "log_event.h"

#include <unistd.h>
#include <fstream>
#include <iostream>

#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/CascadeBG.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumnP.h>

extern "C"
{
#include "xec.h"
}

#ifndef node_H
#include "node.h"
#endif

#ifndef host_H
#include "host.h"
#endif

#include "ecflowview.h"
#undef  XECFDEBUG
#define XECFDEBUG if(0)
#define MENU_REL 1
#define MENU_MAJ 0
#define MENU_MIN 0

#define NUM_MENUS 2
static Widget cmd_menu_popup[NUM_MENUS] = { 0, 0, };
static Widget cmd_menu_name[NUM_MENUS] = { 0, 0, };

static char* defaultMenu[] = {
#include "ecflowview.menu.h"
};

static char* xcdpMenu[] = {
#include "xcdp.menu.h"
};

/* #define YYDEBUG 1 */

class node;
class item;
#include <string>
class menu {
  friend int script_menus(node*, const char *cmd);
	static menu *root_[NUM_MENUS];
        std::string name_;
	item *item_;
	menu *next_;
	Widget widget_;
        int page_;
  static int init(int page, bool def);
public:
  static int num_;

  menu(std::string name, item * i)
    : name_(name), item_(i), next_(0), widget_(0), page_(num_)
  { XECFDEBUG  printf("# menu creation: %s\n", name.c_str());
    if (root_[page_] == NULL) root_[page_] = this; }

  menu *chain(menu * n);
  void create(Widget);
  void update(node *);
  void merge(item *);
	void fill(Widget list,int depth);

  static menu *find(const char *, int page=-1, bool verb=true);
  static void root(menu *m);
};

menu *menu::chain(menu * n)
{
  if (n) { 
    menu *m = menu::find(n->name_.c_str(), n->page_, false);
    if (m) { 
      m->merge(n->item_); delete n;
      XECFDEBUG	printf("# menu already there (chain) %s\n", n->name_.c_str());
    } else if (n->page_ == page_)
      next_ = n;
  }
  return this;
}

void menu::root(menu *m) {
  if (root_[num_] == NULL) root_[num_] = m;
  else if (m) { 
    menu *men = menu::find(m->name_.c_str(), m->page_);
    XECFDEBUG { 
      if (!men) printf("# menu chained %s\n", m->name_.c_str());
      else	  printf("# menu already there %s\n", m->name_.c_str()); }
  }
}

class action {
protected:
  Widget widget_;
  item*  item_;
  
  action() : widget_(0),item_(0) {}
public:
  virtual ~action() {}
  void owner(item* i) { item_ = i; }
  
  virtual void create(Widget, item *) = 0;
  virtual void fill(Widget, int) {}
  
  Widget widget() { return widget_; }
  virtual void run(node *) { }
};

class command : public action {
  char *name_;
  virtual void create(Widget, item *);
public:
  command(char *name): name_(name) { }
  virtual void run(node *);
};

class window_cmd : public action {
  char *name_;
  virtual void create(Widget, item *);
public:
  window_cmd(char *name): name_(name) { }
  virtual void run(node *);
};

typedef void (*nodeproc)(node*);
typedef void (*nodeproc_a_b)(node*, const char* a, const char* b);

class internal: public action {	
  nodeproc proc_;
  virtual void create(Widget, item *);
public:
  internal(nodeproc p): proc_(p) { }
  virtual void run(node *);
};

class internal_a_b: public action {	
  nodeproc_a_b proc_;
  const std::string a_, b_;
  virtual void create(Widget, item *);
public:
  internal_a_b(nodeproc_a_b p, const char* a, const char*b)
  : proc_(p) 
  , a_(a)
  , b_(b)
  { }
  virtual void run(node *);
};


class sub_menu : public action {
        virtual void create(Widget, item *);
	virtual void fill(Widget, int);
public:
	sub_menu() { }
};

class separator : public action {
  virtual void create(Widget, item *);
};

menu *menu::root_[NUM_MENUS] = { 0, 0, };

class item {
public:
  item    *next_;
private:
  friend int script_menus(node*, const char *cmd);
  flags   *visible_;
  flags   *enabled_;
  char    *title_;
  action  *action_;
  char    *question_;
  Boolean answer_;
  int     page_;

public:

  int page() const { return page_; }

  item(flags * visible,flags * enabled,
       char *title, action * action, char *question, Boolean answer)
    : next_(0), visible_(visible), enabled_(enabled)	
    , title_(title), action_(action), question_(question)
    , answer_(answer), page_(menu::num_)
  { if(action_) action_->owner(this); }

  void update(node *);
  
  void create(Widget parent)
  {
    action_->create(parent, this);
    if(next_) 
      next_->create(parent);
  }
  
  item *chain(item * n)
  {
    if (n->page_ == page_)
      next_ = n;
    return this;
  }
  
  char *title() { return title_; }
  
  item *find(const char* name);
  
  void run(node * n)
  {
    str question = n->substitute(question_);
    if(question_[0] == 0 || confirm::ask(answer_,question))
      action_->run(n);
  }
  
  void fill(Widget list,int depth)
  {
    XECFDEBUG printf("# item::fill %p %d\n",list,depth);
    char buf[1024];
    memset(buf,' ',depth);
    sprintf(buf+depth,"%s",title_);
    xec_AddListItem(list,buf);
    action_->fill(list,depth);
    if(next_) 	   
      next_->fill(list,depth);
  }
};

item* item::find(const char* title)
{
  for (item* run=this; run; run=run->next_)
    if (!strcmp(run->title(),title)) return run;
  return 0;
}

menus::menus()
{
}

menus::~menus()
{
}

void menus::write()
{
  int lineno = 0;
  char *line;
  std::ofstream outfile;
  std::string fname = directory::user() + std::string("/ecflowview.menu");
  outfile.open(fname.c_str());
  std::cerr << "# creating menu file " << fname << "\n";
  while ((line = defaultMenu[lineno])) {
    outfile << line << "\n";
    lineno++;
  } 
}

void menus::fillList(Widget list)
{
  XECFDEBUG printf("# menus::fill %p\n",list);
  menu *m = menu::find("MAIN"); if(m) m->fill(list,0);
}

void menu::fill(Widget list,int depth)
{
  item_->fill(list,depth);
}

void menus::show(Widget parent, XEvent * event_node, node * n)
{

  if (parent==NULL) fprintf(stderr, "menus::show null widget\n");
	if(n == 0 || !n->menus())
	{
		selection::menu_node(0);
		return;
	}

	selection::menu_node(n);
	int page = n->__node__() ? 0 : 1;

	if(cmd_menu_popup[page] == 0)
	{
		cmd_menu_popup[page] = XmCreatePopupMenu(parent, "cmd_menu_popup", 0, 0);
		cmd_menu_name[page] = XmCreateLabel(cmd_menu_popup[page], "name", 0, 0);
		Widget w = XmCreateSeparator(cmd_menu_popup[page], "-", 0, 0);

		XtManageChild(cmd_menu_name[page]);
		XtManageChild(w);

		XtAddCallback(cmd_menu_popup[page], XmNentryCallback, menus::entryCB, 0);
		tip::makeTips(cmd_menu_popup[page]);
	}

	menu *m = menu::find("MAIN", page);
	if(m)
	{
	  m->create(cmd_menu_popup[page]);
	  m->update(n);
	}

	xec_VaSetLabel(cmd_menu_name[page], "%s %s",n->type_name(),n->node_name().c_str());
	xec_SetColor(cmd_menu_name[page], n->color(), XmNbackground);

	XmMenuPosition(cmd_menu_popup[page],(XButtonPressedEvent *) event_node);
	XtManageChild(cmd_menu_popup[page]);

}

int menu::num_ = 0;

int menus::version(int rel, int maj, int min) {
  if (rel > MENU_REL || (rel == MENU_REL && maj > MENU_MAJ)) {
    std::cerr << "# menus definition file(s) shall be upgraded\n";
    std::cerr << "# app  is " 
	      << MENU_REL << " " 
	      << MENU_MAJ << " " 
	      << MENU_MIN;
    std::cerr << "\n# file is " << rel << " " << maj << " " << min;
    std::cerr << "\n";

    return 1;
  } // else { std::cout << "# menus version compatible " << rel << " " << maj << " " << min << "\n"; }
  return 0;
}

int menu::init(int page, bool def) {
#ifndef BRIDGE
  page = 0;
#endif
  const char* name = page ? "xcdp.menu" : "ecflowview.menu";
  bool  read = false;
  num_ = page;
  /* 0: ecflowview menu
     1: xcdp       menu

     system\user 0   1
     0          tmp  user
     1          sys  both, user overwrites
  */
  std::string path = directory::user();
  path += "/";
  path += name;
  const char* fname = path.c_str();
  if(!def && !access(fname,F_OK)) { 
    std::cout << "# reading menu file: " << fname << "\n";
    parser::parse(fname); 
    read = true; 
  } else std::cerr << "# menu file not found: " << fname << "\n";
 
  path = directory::system();
  path += "/";
  path += name;
  fname = path.c_str();

  if (!def && !access(fname,F_OK)) {
    std::cout << "# reading menu file: " << fname << "\n";
    parser::parse(fname); 
    read = true; 
  } else std::cerr << "# menu file not found: " << fname << "\n";

  if(!read) {
    char* tmp = getenv("TMPDIR");
    path = tmp ? tmp : "/tmp";
    path += "/";
    path += name;
    fname = path.c_str();

    // if(access(fname,F_OK)) /* create if not already there */
    { /* create always */
      std::cerr << "# creating menu file " << fname << "\n";
      std::ofstream outfile;
      outfile.open(fname);
      int lineno = 0;
      char *line;
      while ((line = (page ? xcdpMenu[lineno] : defaultMenu[lineno]))) {
	outfile << line << "\n";
	lineno++;
      } 
      outfile.close();
    }
    std::cout << "# menu file read: " << fname << "\n";
    parser::parse(fname);
  }
  return TRUE;
}

menu *menu::find(const char *name, int page, bool verb)
{
#ifndef BRIDGE
  page = 0;
#endif
  if (page >= NUM_MENUS || page < 0) page = 0;

  if(root_[page] == NULL) init(page, false);  
  if(root_[page] == NULL) init(page, true);  
  menu *m = root_[page];
  while(m) {
    if(m->name_ == name)
      return m;
    m = m->next_;
  }
  if (verb) printf("# Cannot find menu called %s\n", name);
  return 0;
}

void menu::create(Widget parent)
{
  widget_ = parent;
  XECFDEBUG printf("# menu::create %p %p\n",widget_,parent);
  if(item_) 
      item_->create(parent);
  tip::makeTips(parent);
}

void sub_menu::create(Widget parent, item * i)
{
  if(widget_ == 0) {
    Arg arg;
    Widget sub = XmCreatePulldownMenu(parent, i->title(), 0, 0);
    menu *m = menu::find(i->title(), i->page());
    if(m) 
	m->create(sub);
    XtSetArg(arg, XmNsubMenuId, sub);
    widget_ = XmCreateCascadeButtonGadget(parent, i->title(), &arg, 1);
    XtManageChild(widget_);
    xec_SetUserData(widget_, i);
    XtAddCallback(sub, XmNentryCallback, menus::entryCB, 0);
  }
}

void sub_menu::fill(Widget list,int depth)
{
  menu *m = menu::find(item_->title(), item_->page());
  if(m) 
    m->fill(list,depth+3);
}

void command::create(Widget parent, item * i)
{
  if(widget_ == 0) {
    widget_ = XmCreatePushButtonGadget(parent, i->title(), 0, 0);
    XtManageChild(widget_);
    xec_SetUserData(widget_, i);
  }
}

void window_cmd::create(Widget parent, item * i)
{
  if(widget_ == 0) {
    widget_ = XmCreatePushButtonGadget(parent, i->title(), 0, 0);
    XtManageChild(widget_);
    xec_SetUserData(widget_, i);
  }
}

void internal::create(Widget parent, item * i)
{
  if(widget_ == 0) {
    widget_ = XmCreatePushButtonGadget(parent, i->title(), 0, 0);
    XtManageChild(widget_);
    xec_SetUserData(widget_, i);
  }
}

void internal_a_b::create(Widget parent, item * i) /* internal:: */
{
  if(widget_ == 0) {
    widget_ = XmCreatePushButtonGadget(parent, i->title(), 0, 0);
    XtManageChild(widget_);
    xec_SetUserData(widget_, i);
  }
}

void separator::create(Widget parent, item * i)
{
  if(widget_ == 0) {
    widget_ = XmCreateSeparatorGadget(parent, "-", 0, 0);
    XtManageChild(widget_);
    xec_SetUserData(widget_, i);
  }
}

void menu::update(node * n)
{
  if(item_) item_->update(n);
  if(next_) next_->update(n);
  if(!widget_) return;
  
  int    cnt  = 0;
  Widget last = 0;
  
  CompositeWidget c = CompositeWidget(widget_);
  
  XECFDEBUG printf("# menu::update %d\n",c->composite.num_children);
  
  for(unsigned i = 0 ; i < c->composite.num_children; i++)
    {
      Widget p = c->composite.children[i];
      if(!XtIsManaged(p)) continue;

      XECFDEBUG printf("#    %s ",XtName(p));
      
      if(XtName(p)[0] == '-')
	{
	  XtUnmanageChild(p);
	  last = p;
	  XECFDEBUG printf("# sep\n");
	}
      else
	{
	  if(last)
	    {
	      XECFDEBUG printf("# (%d) ",cnt);
	      if(cnt) XtManageChild(last);
	      cnt  = 0;
	      last = 0;
	    }
	  cnt++;
	  XECFDEBUG printf("# %d\n",cnt);
	}
    }
  XECFDEBUG printf("# menu::update\n");
}

void item::update(node * n)
{
  if(!action_->widget())
    return;
  int page = n->__node__() ? 0 : 1;
#ifndef BRIDGE
  page = 0;
#endif
  if (page_ != page) XtUnmanageChild(action_->widget()); else
  if(visible_->eval(n))
    XtManageChild(action_->widget());
  else
    XtUnmanageChild(action_->widget());
  
  XtSetSensitive(action_->widget(),enabled_->eval(n));
  
  if(next_) next_->update(n);
}

void menus::entryCB(Widget w, XtPointer, XtPointer cb_data)
{
  XmRowColumnCallbackStruct *cb =(XmRowColumnCallbackStruct *) cb_data;
  item *i =(item *) xec_GetUserData(cb->widget);
  if(i && selection::menu_node())
    i->run(selection::menu_node());
}

void command::run(node* n)
{
  n->command(name_);
}

void window_cmd::run(node* n)
{
  if (n != 0 && strncmp("Collect", name_, 7) == 0) collector::show(*n); else
  panel_window::new_window(n,name_,true,true);
}

void internal::run(node* n)
{
  proc_(n);
}

void internal_a_b::run(node* n)
{
  proc_(n, a_.c_str(), b_.c_str());
}

void  menus::root(menu* m)
{
  menu::root(m);
}

menu* menus::chain(menu* a,menu* b)
{
  return a->chain(b);
}

item* menus::chain(item* a,item* b)
{
  return a->chain(b);
}

menu* menus::create(char* a,item* b)
{
  XECFDEBUG printf("# create menus %s\n", a);
  return new menu(a,b);
}

item* menus::create(flags* a,flags* b,char* c,action* d,char* e,bool f)
{
  XECFDEBUG printf("# create item %d %d %s %s %d\n", 
		   a->eval(0) ? 1 : 0, b->eval(0) ? 1 : 0,
		   c, e, f ? 1:0);
  return new item(a,b,c,d,e,f);
}

action* menus::command(char* a)
{
  return new ::command(a);
}

action* menus::separator()
{
  return new ::separator();
}

action* menus::sub_menu()
{
  return new ::sub_menu();
}

action* menus::window(char* a)
{
  return new ::window_cmd(a);
}

action* menus::internal(nodeproc a)
{
  return new ::internal(a);
}

action* menus::internal_a_b(nodeproc_a_b a, const char*b, const char *c)
{
  return new ::internal_a_b(a, b, c);
}

void menu::merge(item *i) {
  if (!i) return; 
  /* menu is old original menu, 
     its items are moved away, 
     new items take place
     only the old items not found are kept */
  if (!item_) { item_ = i; return; }
  item *run, *next, *old = item_, *last = i; 
  while (last->next_) { last = last->next_; }
  item_ = i; 
  for (run = old; run; ) {
    next = run->next_; run->next_ = 0;
    if (!item_->find(run->title())) { last->next_ = run; last = run; }
    run = next;
  }
  last->next_ = 0;
}

extern "C" {
// The stuff is here because in some os, lex and yacc are broken with c++

int   menus_version(int rel, int maj, int min) { return menus::version(rel, maj, min); }
menu* menus_chain_menus(menu *a,menu *b) { return menus::chain(a,b); }
item* menus_chain_items(item *a,item *b) { return menus::chain(a,b); }
void  menus_root(menu *m)                { menus::root(m); }
menu* menus_create_2(char* a,item* b)    { return menus::create(a,b); }
item* menus_create_6(flags* a,flags* b,char* c,action* d,char* e,int f)
  { return menus::create(a,b,c,d,e,f); }

action* menus_command(char* a) { return  menus::command(a); }
action* menus_window(char* a)  { return  menus::window(a); }
action* menus_separator()      { return  menus::separator(); }
action* menus_sub_menu()       { return  menus::sub_menu(); }

flags* new_flagNone()                  { return new flagNone(); }
flags* new_typeFlag(int a)             { return new typeFlag(a); }
flags* new_flagNot(flags* a)           { return new flagNot(a); }
flags* new_flagOr(flags* a,flags* b)   { return new flagOr(a,b); }
flags* new_flagAnd(flags* a,flags* b)  { return new flagAnd(a,b); }

flags* new_flagAll()                   { return new flagAll(); }
flags* new_eventFlag(int a)            { return new eventFlag(a); }
flags* new_selectionFlag()             { return new selectionFlag(); }
flags* new_statusFlag(int a)           { return new statusFlag(a); }
flags* new_userFlag(int a)             { return new userFlag(a); }

flags* new_procFlag_node_hasTriggers() { return new procFlag(&node::hasTriggers); }
flags* new_procFlag_node_hasDate() { return  new procFlag(&node::hasDate); }
flags* new_procFlag_node_hasTime() { return  new procFlag(&node::hasTimeHolding); }
flags* new_procFlag_node_isMigrated() { return new procFlag(&node::isMigrated); }
flags* new_procFlag_node_isLocked() { return  new procFlag(&node::isLocked); }
flags* new_procFlag_node_hasText() { return  new procFlag(&node::hasText); }
flags* new_procFlag_node_isZombie() { return  new procFlag(&node::isZombie); }

action* menus_internal_host_plug()  { return menus::internal(&host::plug); }
action* menus_internal_host_comp(const char*a, const char*b)  { 
  return menus::internal_a_b(&host::comp, a, b); }

void log_event_meter_event(const DateTime& a,node* b,int c) { 
  log_event::meter_event(a,b,c); }
void log_event_event_event(const DateTime& a,node* b,int c) { 
  log_event::event_event(a,b,c); }
void log_event_status_event(const DateTime& a,node* b,int c) { 
  log_event::status_event(a,b,c); }
const node *log_event_find(char* a) { return log_event::find(a); }
}

int script_menus(node*, const char *cmd)
{
  menu *m = menu::find("MAIN");
  if (!m) { std::cerr << "# no menu available!"; return 1; }
  node *n = selection::current_node();
  const char* arg = cmd + 5;
  unsigned int size = arg? strlen(arg) : 0;
  if (!n) { std::cerr << "# no node selected!"; return 1; }
  while (m) {
    item *i = m->item_;
    while (i) {
      if (i->visible_ && i->visible_->eval(n)) {
	if (i->enabled_ && i->enabled_->eval(n)) {
	  if (i->action_) {
	    if (size && !strncasecmp(arg, i->title_, size)) {
	      std::cout << "# cmd issued:   " << i->title_ << "\n";
	      i->action_->run(n);
	    } else 
	      std::cout << "# item:         " << i->title_ << "\n";
	  } else
	    std::cout << "# item enabled: " << i->title_ << "\n";
	} else
	  std::cout <<   "# item visible: " << i->title_ << "\n";      
      }
      i = i->next_;
    }
    m = m->next_;
  }

  return 0;
}

