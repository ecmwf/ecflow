//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #114 $ 
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

#include "ecf_node.h"
#include "host.h"
#include "node.h"
#include "tree.h"
#include "ChangeMgrSingleton.hpp"
#include "NodeAttr.hpp"
#include "Variable.hpp"
#include "dummy_node.h"
#include "external.h"
#include <Str.hpp> 
#ifndef NODE_MAX
#define NODE_MAX 41
#endif

#include <iostream>
#include <locale>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
static int nb_tasks = 0;
static int nb_attrs = 0;
static char* info_label = getenv("ECFLOWVIEW_INFO_LABEL");

std::map<std::string, ecf_node_maker*>& ecf_node_maker::map()
{
   static std::map<std::string, ecf_node_maker*> map_;
   return map_;
}

std::vector<ecf_node_maker*>& ecf_node_maker::builders()
{
   static std::vector<ecf_node_maker*> builders_((size_t)NODE_MAX,(ecf_node_maker*)0);
   return builders_;
}

void ecf_node::counter() {
  int count = 0; node* n = 0x0; if (node_) n = node_->kids();
  while (n) { 
    std::cerr << "# " << n->full_name() << " " << n->type() << "\n";
    n = n->next(); 
    count++; 
  } 
  if (count) { 
    std::cerr << "# " << full_name() << " kids: " << count << "\n"; 
  }
}

void ecf_node::update(const Node* n, const std::vector<ecf::Aspect::Type>&)
{
  if (!node_) return;
  /* ok node create through node replace is simple update */
  node_->update(-1, -1, -1); 
  node_->notify_observers(); 
  node_->redraw();
}

int convert(NState::State state) {
   int rc = STATUS_UNKNOWN;
   switch (state) {
      case NState::UNKNOWN :  rc= STATUS_UNKNOWN;  break;
      case NState::COMPLETE:  rc= STATUS_COMPLETE; break;
      case NState::QUEUED:    rc= STATUS_QUEUED;   break;
      case NState::ABORTED:   rc= STATUS_ABORTED;  break;
      case NState::SUBMITTED: rc= STATUS_SUBMITTED;break;
      case NState::ACTIVE:    rc= STATUS_ACTIVE;   break;
      default: rc = STATUS_UNKNOWN;  break;
   }
   return rc;
}

int convert(DState::State state) {
   int rc = STATUS_UNKNOWN;
   switch (state) {
      case DState::UNKNOWN :  rc= STATUS_UNKNOWN;  break;
      case DState::COMPLETE:  rc= STATUS_COMPLETE; break;
      case DState::QUEUED:    rc= STATUS_QUEUED;   break;
      case DState::ABORTED:   rc= STATUS_ABORTED;  break;
      case DState::SUBMITTED: rc= STATUS_SUBMITTED;break;
      case DState::ACTIVE:    rc= STATUS_ACTIVE;   break;
      default: rc = STATUS_UNKNOWN;  break;
   }
   return rc;
}

void ecf_node::update(const Defs*, const std::vector<ecf::Aspect::Type>&)
{
  if (!node_) return;
  node_->update(-1, -1, -1); 
  node_->notify_observers(); 
  node_->redraw();
}

void ecf_node::update_delete(const Node* n) { 
  if (!node_) return;
  node_->unlink();
  node *parent = node_->parent();
  node_->visibility(False);
  node_->remove();
  delete node_;
  node_ = 0x0; 
  notify_observers();
  if (parent) { 
    parent->folded_ = true;
    parent->update(-1, -1, -1); 
    parent->notify_observers(); 
    parent->redraw(); 
  }
}

void ecf_node::update_delete(const Defs* n) {
  if (node_) node_->unlink(); 
  node_ = 0x0; 
  notify_observers();
}

void ecf_node::unlink(bool detach) { 
  if (detach) {}
  else if (node_) 
    node_->unlink(); 
  node_ = 0x0; 
}

const Repeat& ecf_node::crd() { static const Repeat REPEAT = Repeat( RepeatInteger("PRB", 1, 1, 1) ); return REPEAT; }

node* ecf_node_maker::make_xnode(host& h, ecf_node* n, std::string type)
{ 
   if (!n) return NULL;
   node* out = NULL;

   if (n->type() >= 0 && n->type() < NODE_MAX && builders()[n->type()]) {
      if (n->type() == NODE_REPEAT)
         out = map()[type]->make(h, n);
      else
         out = builders()[n->type()]->make(h, n);
      n->set_graphic_ptr(out);
   }
   else {
      std::cout << "!!!" << n->full_name() << n->type() << " " << n->name() << " " << n->type_name() << "\n";
      if (map()[type]) {
         out = map()[type]->make(h, n);
         assert(out);
         n->set_graphic_ptr(out);
         std::cout << "!!!ok\n";
      }
   }
   return out;
}

ecf_node::ecf_node(ecf_node* parent, const std::string& name, char k) 
  : parent_(parent), node_(0), kind_(k) 
  , name_(name)
  , trigger_(0)
  , complete_(0)
{
}

ecf_node::~ecf_node() 
{  
  // std::cerr << "# eode del: " << full_name_ << std::endl;
  nokids(true);
  unlink();
  delete trigger_;
  delete complete_;
}

node* ecf_node::create_tree(host& h, node* xnode) {
  if (xnode) { node_ = xnode; }
  else if (node_) return node_;
  else if (!(node_ = create_node(h))) { return 0x0; }
  if (get_node()) 
    get_node()->set_graphic_ptr(node_);
  
  for(std::vector<ecf_node*>::const_iterator 
        j  = kids_.begin(); j != kids_.end(); ++j) 
    if (*j) 
      node_->insert((*j)->create_tree(h, 0x0));
  
  return node_;
}

void ecf_node::add_kid(ecf_node* k) {
  if (k) {
    kids_.push_back(k);

    if (k->type() == NODE_TASK) { 
      nb_tasks++;
      kids_.push_back(make_node(new Label("time", to_simple_string(k->status_time())), k));
    } else if (k->type() == NODE_FAMILY) {} 
    else nb_attrs++;
  }
}

template<> const std::string& ecf_concrete_node<Defs>::name() const
{ return ecf_node::slash(); }

template<> 
void ecf_concrete_node<Node>::why(std::ostream&f) const
{   
   if (!owner_) return;
   std::vector<std::string> theReasonWhy;
   std::vector<std::string>::const_iterator it;
   owner_->bottom_up_why(theReasonWhy); 
   for (it=theReasonWhy.begin(); it != theReasonWhy.end(); ++it)
     f << (*it) << "\n";
 }


template<> 
void ecf_concrete_node<Suite>::why(std::ostream &f) const
{   
   if (!owner_) return;
   std::vector<std::string> theReasonWhy;
   std::vector<std::string>::const_iterator it;
   owner_->bottom_up_why(theReasonWhy); 
   for (it=theReasonWhy.begin(); it != theReasonWhy.end(); ++it)
     f << (*it) << "\n";
 }

template<> 
void ecf_concrete_node<Defs>::why(std::ostream &f) const
{   
   if (!owner_) return;
   std::vector<std::string> theReasonWhy;
   std::vector<std::string>::const_iterator it;
   owner_->why(theReasonWhy); 
   for (it=theReasonWhy.begin(); it != theReasonWhy.end(); ++it)
     f << (*it) << "\n";
}

#define UNLINK(T) template<> void ecf_concrete_node<T>::unlink(bool detach) \
{ if (!owner_) return; if (detach) ChangeMgrSingleton::instance()->detach(owner_,this); owner_ = 0x0; }
UNLINK(Alias)
UNLINK(Task)
UNLINK(Family)
UNLINK(Suite)
UNLINK(Node)
UNLINK(Defs)
#undef UNLINK

#define SORT_VAR 1
#ifdef SORT_VAR
struct cless_than {
    inline bool operator() (const Variable& v1,  const Variable& v2)    {
      return (v1.name() < v2.name()); } 
};
#endif

template<> 
void ecf_concrete_node<Alias>::make_subtree() {

  if (!owner_) return;
  Alias* n = owner_; 

  full_name_ = owner_->absNodePath();

  ChangeMgrSingleton::instance()->attach(owner_, this);  
  n->update_generated_variables();
  
  std::vector<Variable> gvar; 
  n->gen_variables(gvar);
  std::vector<Variable>::const_iterator it;
#ifdef SORT_VAR
  std::sort(gvar.begin(),gvar.end(),cless_than());
#endif

  for (it = gvar.begin(); it != gvar.end(); ++it)
    if ((*it).name() == "" || !(*it == Variable::EMPTY()))
      add_kid(make_node(*it, this, 'g'));
    else std::cerr << "# empty variable\n";
  
#ifdef SORT_VAR
  gvar = n->variables(); /* expensive */
  std::sort(gvar.begin(),gvar.end(),cless_than());
  make_kids_list(this,gvar);
#else
  make_kids_list(this,n->variables());
#endif

  make_kids_list(this,n->labels());
  make_kids_list(this,n->events());
  make_kids_list(this,n->meters());
}

template<> 
void ecf_concrete_node<Node>::make_subtree() {

  if (!owner_) return;
  Node* n = owner_; 

  full_name_ = owner_->absNodePath();
  ChangeMgrSingleton::instance()->attach(owner_, this);  

  if (owner_->suite()->begun())
    owner_->update_generated_variables();

  std::vector<node_ptr> kids; n->immediateChildren(kids);
  make_kids_list(this,kids);
  
  std::vector<Variable> gvar; 
  n->gen_variables(gvar);
#ifdef SORT_VAR
  std::sort(gvar.begin(),gvar.end(),cless_than());
#endif

  std::vector<Variable>::const_iterator it;
  for (it = gvar.begin(); it != gvar.end(); ++it)
    if (!(*it == Variable::EMPTY()))
      add_kid(make_node(*it, this, 'g'));
    else std::cerr << "# empty variable\n";

#ifdef SORT_VAR
  gvar = n->variables(); /* expensive */
  std::sort(gvar.begin(),gvar.end(),cless_than());
  make_kids_list(this,gvar);
#else
  make_kids_list(this,n->variables());
#endif

  make_kids_list(this,n->labels());
  make_kids_list(this,n->events());
  make_kids_list(this,n->meters());

  make_kids_list(this,n->timeVec());
  make_kids_list(this,n->todayVec());
  make_kids_list(this,n->crons());
  
  make_kids_list(this,n->dates());
  make_kids_list(this,n->days());
    
  make_kids_list(this,n->limits());
  make_kids_list(this,n->inlimits());
  
  if (n->get_trigger()) { 
    trigger_ = new ExpressionWrapper(n, 't');
    add_kid(make_node(trigger_, this, 't'));
  }
  if (n->get_complete()) {
    complete_ = new ExpressionWrapper(n, 'c');
    add_kid(make_node(complete_, this, 'c'));
  }

  if (n->get_late() != 0x0) { add_kid(make_node(n->get_late(), this)); }

  if ((!n->repeat().empty()) && "" != n->repeat().name()) {
    RepeatEnumerated *re;
    RepeatDate *rd;
    RepeatString *rs;
    RepeatInteger *ri;
    RepeatDay *rday;

    if ((re = dynamic_cast<RepeatEnumerated *>(n->repeat().repeatBase())))
      add_kid(make_node(re, this));
    else if ((rd = dynamic_cast<RepeatDate *>(n->repeat().repeatBase())))
      add_kid(make_node(rd, this)); 
    else if ((rs = dynamic_cast<RepeatString *>(n->repeat().repeatBase())))
      add_kid(make_node(rs, this));
    else if ((ri = dynamic_cast<RepeatInteger *>(n->repeat().repeatBase())))
      add_kid(make_node(ri, this));
    else if ((rday = dynamic_cast<RepeatDay *>(n->repeat().repeatBase())))
      {}
    else 
      std::cerr << "# ecflfowview does not recognises this repeat item\n";
  }
}

template<> 
void ecf_concrete_node<Suite>::make_subtree() {

  if (!owner_) return;
  Suite* n = owner_; 

  nb_tasks = 0;
  nb_attrs = 0;

  if (n->begun())
    n->update_generated_variables();

  full_name_ = owner_->absNodePath(); // "/" + n->name();

  ChangeMgrSingleton::instance()->attach(owner_, this); 

  std::vector<node_ptr> kids; n->immediateChildren(kids);
  make_kids_list(this,kids);
  
  std::vector<Variable> gvar;
  n->gen_variables(gvar);  

  std::vector<Variable>::const_iterator it;
  for (it = gvar.begin(); it != gvar.end(); ++it)
    if (!(*it == Variable::EMPTY()))
      add_kid(make_node(*it, this, 'g'));
    else std::cerr << "# empty variable\n";

  std::string info = "";
#ifdef SORT_VAR
  gvar = n->variables(); /* expensive */
  std::sort(gvar.begin(),gvar.end(),cless_than());
  make_kids_list(this,gvar);
  if (info_label)
    for (it=gvar.begin(); it!=gvar.end(); ++it) {
      // if ((*it).name().find("HOST") != std::string::npos) info += ", " + (*it).theValue(); else 
      if ("ECF_JOB_CMD" == (*it).name() || 
	  "HOST" == (*it).name() || 
	  "WSHOST" == (*it).name() || 
	  "SCHOST" == (*it).name())
	info += ", " + (*it).theValue();
    }
#else
  make_kids_list(this,n->variables());
#endif

  make_kids_list(this,n->labels());
  make_kids_list(this,n->events());
  make_kids_list(this,n->meters());
  
  make_kids_list(this,n->timeVec());
  make_kids_list(this,n->todayVec());
  make_kids_list(this,n->crons());
  
  make_kids_list(this,n->dates());
  make_kids_list(this,n->days());
  
  make_kids_list(this,n->limits());
  make_kids_list(this,n->inlimits());
  
  if (n->get_trigger()) { 
    trigger_ = new ExpressionWrapper(n, 't');
    add_kid(make_node(trigger_, this, 't'));
  }
  if (n->get_complete()) {
    complete_ = new ExpressionWrapper(n, 'c');
    add_kid(make_node(complete_, this, 'c'));
  }

  if (n->get_late() != 0x0) { add_kid(make_node(n->get_late(), this)); }
  if ((!n->repeat().empty())) {
    RepeatEnumerated *re;
    RepeatDate *rd;
    RepeatString *rs;
    RepeatInteger *ri;
    RepeatDay *rday;

    if ((re = dynamic_cast<RepeatEnumerated *>(n->repeat().repeatBase())))
      add_kid(make_node(re, this));
    else if ((rd = dynamic_cast<RepeatDate *>(n->repeat().repeatBase())))
      add_kid(make_node(rd, this)); 
    else if ((rs = dynamic_cast<RepeatString *>(n->repeat().repeatBase())))
      add_kid(make_node(rs, this));
    else if ((ri = dynamic_cast<RepeatInteger *>(n->repeat().repeatBase())))
      add_kid(make_node(ri, this));
    else if ((rday = dynamic_cast<RepeatDay *>(n->repeat().repeatBase())))
      add_kid(make_node(rday, this));
    else 
      std::cerr << "# ecflfowview does not recognises this repeat item\n";
    }

  /* INT-67 */
  if (info_label)
    try {
      char msg[400]; 
      snprintf(msg,400, "nb_tasks %d, nb_attrs %d%s", 
	       nb_tasks, nb_attrs, info.c_str());
      const Label * labt = new Label("info", msg);
      add_kid(make_node(labt, this));
      XECFDEBUG {
	std::cout << "#MSG suite " << this->name() << msg << "\n";
      } } catch(...) {} 
}

template<>
const std::string& ecf_concrete_node<Defs>::variable(const std::string& name) const
{ 
  Defs* n = owner_;
  if (!n) return ecf_node::none();

  const Variable & var = owner_->server().findVariable(name);
  if (!var.empty())
    return var.theValue();

  return ecf_node::none();
}

template<>
const std::string& ecf_concrete_node<Node>::variable(const std::string& name) const
{
  if (!owner_) 
    return ecf_node::none();

  const Variable & var = owner_->findVariable(name);
  if (!var.empty())
    return var.theValue();

  return ecf_node::none();
}

template<>
const std::string& ecf_concrete_node<Suite>::variable(const std::string& name) const
{
  if (!owner_) 
    return ecf_node::none();

  const Variable& var = owner_->findVariable(name);
  if (!var.empty())
    return var.theValue();

  return ecf_node::none();
}

template<> ecf_concrete_node<const Variable>::
ecf_concrete_node(const Variable *owner, ecf_node *parent, const char c)
  : ecf_node(parent, owner->name(), c)
  , owner_(owner)
{
}

template<> 
const std::string& ecf_concrete_node<const Event>::full_name() const
{
  full_name_ = parent()->full_name();
  full_name_ += ":";
  full_name_ += name();
  return full_name_;
}
template<> 
const std::string& ecf_concrete_node<const Meter>::full_name() const
{
  full_name_ = parent()->full_name();
  full_name_ += ":";
  full_name_ += name();
  return full_name_;
}
template<> 
const std::string& ecf_concrete_node<const Repeat>::full_name() const
{
  full_name_ = parent()->full_name();
  full_name_ += ":";
  full_name_ += name();
  return full_name_;
}
template<> 
const std::string& ecf_concrete_node<const Variable>::full_name() const
{
  full_name_ = parent()->full_name();
  full_name_ += ":";
  full_name_ += name();
  return full_name_;
}

template<> 
const std::string& ecf_concrete_node<const std::pair<std::string, std::string> >
::full_name() const
{
  full_name_ = parent()->full_name();
  full_name_ += ":";
  full_name_ += name();
  return full_name_;
}

template<> 
ecf_concrete_node<const std::pair<std::string, std::string> >::
ecf_concrete_node(const std::pair<std::string, std::string> *owner, 
                  ecf_node *parent, const char c)
  : ecf_node(parent, owner->first, c)
  , owner_(owner)
{
}

template<>
std::ostream& ecf_concrete_node<Defs>::print(std::ostream& s) const 
{ 
  if (owner_) 
    owner_->print(s); 
  return s; 
}

template<>
std::ostream& ecf_concrete_node<Node>::print(std::ostream& s) const 
{ 
  if (owner_) 
    owner_->print(s); 
  return s; 
}

template<>void ecf_concrete_node<Defs>::print(std::ostream& s){ }
template<>void ecf_concrete_node<Node>::print(std::ostream& s){ }

template<> const std::string& ecf_concrete_node<Defs>::full_name() const
{ return ecf_node::slash(); }

void hide(node* n) {
  while (n) {
    n->visibility(False);
    hide(n->kids());
    n = n->next();
  }
}

void ecf_node::nokids(bool own) { 
  if (node_) { node::destroy(node_->kids_); node_->kids_ = 0x0; }

  for (size_t i = 0; i < kids_.size(); i++) {
     delete kids_[i];
  }
  kids_.clear();
}

int redraw_kids(node* node_, 
		const std::vector<ecf::Aspect::Type>& aspect) 
{
   int tot = 0;
   for(std::vector<ecf::Aspect::Type>::const_iterator it = aspect.begin(); it != aspect.end(); ++it) {
     int  kind = 0;
     switch ( *it ) {
         case ecf::Aspect::METER:
	   kind = NODE_METER;
            break;
         case ecf::Aspect::EVENT:
	   kind = NODE_EVENT;
            break;
         case ecf::Aspect::LABEL: 	   
	   kind = NODE_LABEL;
            break;
         case ecf::Aspect::LIMIT:
	   kind = NODE_LIMIT;
	   break;
         case ecf::Aspect::REPEAT:
	   kind = NODE_REPEAT;
	   // node_->update(-1, -1, -1); node_->redraw(); break;
         case ecf::Aspect::STATE:
	   node_->update(-1, -1, -1); node_->redraw();
	   break;
         case ecf::Aspect::SERVER_VARIABLE:
#undef NODE_VARIABLE
     case ecf::Aspect::NODE_VARIABLE:
#define NODE_VARIABLE 3
       kind = NODE_VARIABLE;
      default: 
	   continue;
      }
      ++tot;
      if (kind)
	   for(node *xn = node_->kids(); xn; xn = xn->next())
	     if (xn) if (xn->type() == kind) {
		 xn->update(-1, -1, -1);
		 xn->redraw();
	       }
   }
   return tot;
}

void update_status_time(node* xnode, const Node* n, ecf_node* ecf) {
  if (!n) return;  
  if (!ecf) return;
  if (!ecf->type() == NODE_TASK) return;
  if (!xnode) return;
  /*
  for (std::vector<ecf_node*>::iterator it = ecf->kids_.begin(); it != ecf->kids_.end(); ++it)
    if ((*it)->type() == NODE_LABEL && (*it)->name() == "time") {
      Label* lb = dynamic_cast<Label *>(n->findLabel("time"));
      if (lb) lb->set_new_value(to_simple_string(ecf->state_time())); */
}

template<> 
void ecf_concrete_node<Node>::update(const Node* n, 
                                     const std::vector<ecf::Aspect::Type>& aspect)
{
   if (!owner_) return;
   if (!node_) return;
   if (is_reset(aspect)) {
      Updating::set_full_redraw();
      return;
   }
   
   node_->delvars();
      if (owner_->suite()->begun()) {
         owner_->update_generated_variables();
      }
      std::vector<Variable> gvar;
      n->gen_variables(gvar);
      std::vector<Variable>::const_iterator it, gvar_end;
      for(it = gvar.begin(); it != gvar.end(); ++it) {
         if ((*it).name() == "" || *it == Variable::EMPTY()) {
            std::cerr << "# empty variable\n";
            continue;
         }
         ecf_node *run = make_node(*it, this, 'g');
         add_kid(run);
         node_->insert(run->create_node(node_->serv()));
      }

#ifdef SORT_VAR
      gvar = n->variables(); /* expensive */
      std::sort(gvar.begin(), gvar.end(), cless_than());
      gvar_end = gvar.end();
      for(it = gvar.begin(); it != gvar_end; ++it) {
#else
      for (it = n->variables().begin(); it != n->variables().end(); ++it) {
#endif
         if ((*it).name() == "" || *it == Variable::EMPTY()) {
            std::cerr << "# empty variable\n";
            continue;
         }
         ecf_node *run = make_node(*it, this);
         add_kid(run);
         node_->insert(run->create_node(node_->serv()));      
      }

   const_cast<Node*>(n)->set_graphic_ptr(xnode());
   if (redraw_kids(node_, aspect) == 1) return;

   if (info_label) update_status_time(node_, n, this);

   node_->update(-1, -1, -1); // call pop up window with check
   node_->notify_observers();
   node_->redraw();
}

template<> 
void ecf_concrete_node<Suite>::update(const Node* n, 
                                      const std::vector<ecf::Aspect::Type>& aspect) 
{
  if (!owner_) return;     
  if (!node_) return;
  assert(xnode());
  const_cast<Node*> (n)->set_graphic_ptr(xnode()); /* ??? */

  if (is_reset(aspect)) {  

     Updating::set_full_redraw();
     return;
  }

  if (owner_->begun())
     owner_->update_generated_variables();

  if (redraw_kids(node_, aspect) == 1) return;

  node_->update(-1, -1, -1);
  node_->notify_observers();
  node_->redraw();
}

template<>
bool ecf_concrete_node<Suite>::is_late() { 
  if (!owner_) return false;
  return owner_->get_late()
    ? owner_->get_late()->isLate()
    : false; 
}

template<>
bool ecf_concrete_node<Node>::is_late() {
  if (!owner_) return false;
  return owner_->get_late()
    ? owner_->get_late()->isLate()
    : false; 
}

template<>
bool ecf_concrete_node<Suite>::hasZombieAttr() { 
  if (!owner_) return false;
  return owner_->zombies().size() > 0;
}

template<>
bool ecf_concrete_node<Node>::hasZombieAttr() {
  if (!owner_) return false;
  return owner_->zombies().size() > 0;
}

template<>
bool ecf_concrete_node<Node>::hasTime() {
  return owner_ ? (owner_->timeVec().size() > 0 ||
                   owner_->todayVec().size() > 0 ||
                   owner_->crons().size() > 0) 
    : false;
}

template<>
bool ecf_concrete_node<Suite>::hasTime() {
  return owner_ ? (owner_->timeVec().size() > 0 ||
                   owner_->todayVec().size() > 0 ||
                   owner_->crons().size() > 0) 
    : false;
}

template<>
bool ecf_concrete_node<Family>::hasTime() {
  return owner_ ? (owner_->timeVec().size() > 0 ||
                   owner_->todayVec().size() > 0 ||
                   owner_->crons().size() > 0) 
    : false;
}

template<>
bool ecf_concrete_node<Node>::hasTrigger() {
  return owner_ ? (owner_->triggerAst() || 
                   owner_->completeAst()) 
    : false;
}

template<>
bool ecf_concrete_node<Node>::hasDate() {
  return owner_ ? (owner_->days().size() > 0 || 
                   owner_->dates().size() > 0) 
    : false;
}

template<>
bool ecf_concrete_node<Suite>::hasDate() {
  return owner_ ? (owner_->days().size() > 0 || 
                   owner_->dates().size() > 0) 
    : false;
}

template<>
bool ecf_concrete_node<Family>::hasDate() {
  return owner_ ? (owner_->days().size() > 0 || 
                   owner_->dates().size() > 0) 
    : false;
}


template<> ecf_concrete_node<const Event>::
ecf_concrete_node(const Event* owner,ecf_node* parent, const char c) 
   :  ecf_node(parent, owner ? owner->name_or_number() : ecf_node::none(), c)
   , owner_(owner)
{}

template<> 
const std::string ecf_concrete_node<ExpressionWrapper>::toString() const 
  { if (owner_) return owner_->expression(); return ecf_node::none(); }

template<> 
const std::string ecf_concrete_node<Expression>::toString() const
{ if (owner_) return owner_->expression();
  return ecf_node::none(); }

template<> 
const std::string ecf_concrete_node<external>::toString() const 
{ return ecf_node::none(); }

template<> 
const std::string ecf_concrete_node<dummy_node>::toString() const 
{ return ecf_node::none(); }

template<> 
const std::string ecf_concrete_node<AstTop>::toString() const 
{ return owner_ ? owner_->expression() : ecf_node::none(); }

template<> 
const std::string ecf_concrete_node<Suite>::toString() const 
{ return owner_ ? owner_->name() : ecf_node::none(); }

template<> 
const std::string ecf_concrete_node<Node>::toString() const 
{ return owner_ ? owner_->name() : ecf_node::none(); }

template<> 
const std::string ecf_concrete_node<Defs>::toString() const
{ return ecf_node::slash(); }

template<> 
const std::string ecf_concrete_node<RepeatDay>::toString() const
{ if (parent())
      return parent()->get_repeat().toString();
  return none();
}

/*template<> 
const std::string ecf_concrete_node<Label>::toString() const
{ if (parent())
      return parent()->get_label().toString();
  return none();
  }*/

template<> 
const std::string ecf_concrete_node<RepeatDate>::toString() const
{ if (parent())
      return parent()->get_repeat().toString();
  return none();
}

template<> 
const std::string ecf_concrete_node<RepeatEnumerated>::toString() const
{ if (parent())
      return parent()->get_repeat().toString();
  return none();
}

template<> 
const std::string ecf_concrete_node<RepeatString>::toString() const
{ if (parent())
      return parent()->get_repeat().toString();
  return none();
}

template<> 
const std::string ecf_concrete_node<RepeatInteger>::toString() const
{ if (parent())
      return parent()->get_repeat().toString();
  return none();
}

template<>
const std::string ecf_concrete_node<const std::pair<std::string, std::string> >
::toString() const
{ if (owner_) return owner_->first + " : " + owner_->second; return "pair"; }

template<> int ecf_concrete_node<const Event>::status() const
{ return owner_ ? owner_->value() : 0; }

template<> boost::posix_time::ptime ecf_concrete_node<Suite>::status_time() const
{   
  if (owner_) return owner_->state_change_time(); 
  return boost::posix_time::ptime(); 
}

template<> boost::posix_time::ptime ecf_concrete_node<Family>::status_time() const
{   
  if (owner_) return owner_->state_change_time(); 
  return boost::posix_time::ptime(); 
}

template<> boost::posix_time::ptime ecf_concrete_node<Node>::status_time() const
{ 
  if (owner_) return owner_->state_change_time(); 
  return boost::posix_time::ptime(); 
}

template<> int ecf_concrete_node<Suite>::status() const
{
  int rc = STATUS_UNKNOWN;
  if (!owner_) return rc;
  else if (!owner_->begun()) return rc;
  else rc = convert(owner_->state());
  return owner_->isSuspended() ? STATUS_SUSPENDED : rc;
}

template<> int ecf_concrete_node<Node>::status() const
{
  int rc = STATUS_UNKNOWN;
  if (!owner_) return rc;
  else rc = convert(owner_->state());
  return owner_->isSuspended() ? STATUS_SUSPENDED : rc;
}

template<> int ecf_concrete_node<Defs>::status() const
{
  int rc = STATUS_UNKNOWN;
  if (!owner_) return rc;
  switch (owner_->server().get_state()) {
  case SState::HALTED:   rc = STATUS_HALTED;   break;
  case SState::SHUTDOWN: rc = STATUS_SHUTDOWN; break;
  case SState::RUNNING: rc = convert(owner_->state()); break;
  } 
  return rc;
}

template<> int ecf_concrete_node<Suite>::defstatus() const
{
  return owner_ ? convert(owner_->defStatus()) : STATUS_QUEUED;
}

template<> int ecf_concrete_node<Node>::defstatus() const
{
  return owner_ ? convert(owner_->defStatus()) : STATUS_QUEUED;
}

template<>int ecf_concrete_node<Defs>::flags() const { 
  return owner_ ? owner_->flag().flag() : 0;
}

template<>int ecf_concrete_node<Node>::flags() const { 
  return owner_ ? owner_->flag().flag() : 0;
}
template<>int ecf_concrete_node<Suite>::flags() const { 
  return owner_ ? owner_->flag().flag() : 0;
}

template<>int ecf_concrete_node<Node>::type() const { 
  int rc = NODE_UNKNOWN;
  if (!owner_) return rc;
  if (owner_->isFamily()) rc = NODE_FAMILY;
  else if (owner_->isAlias())  rc = NODE_ALIAS;
  else if (owner_->isTask())   rc = NODE_TASK;
  else if (owner_->isSuite())  rc = NODE_SUITE;
  return rc;
}

template<>int ecf_concrete_node<Alias>::type() const { return NODE_ALIAS; }
template<>int ecf_concrete_node<Suite>::type() const { return NODE_SUITE; }
template<>int ecf_concrete_node<Family>::type() const { return NODE_FAMILY; }
template<>int ecf_concrete_node<Defs>::type() const { return NODE_SUPER; }
template<>int ecf_concrete_node<const ecf::TimeAttr>::type() const { return NODE_TIME; }
template<>int ecf_concrete_node<const ecf::TodayAttr>::type() const { return NODE_TIME; }
template<>int ecf_concrete_node<const ecf::CronAttr>::type() const { return NODE_TIME; }
template<>int ecf_concrete_node<const DateAttr>::type() const { return NODE_DATE; }
template<>int ecf_concrete_node<DateAttr>::type() const { return NODE_DATE; }
template<>int ecf_concrete_node<const DayAttr>::type() const { return NODE_DATE; }
// template<>int ecf_concrete_node<RepeatBase>::type() const { return NODE_REPEAT; }
template<>int ecf_concrete_node<RepeatEnumerated>::type() const { return NODE_REPEAT_E; }
template<>int ecf_concrete_node<RepeatString>::type() const     { return NODE_REPEAT_S; }
template<>int ecf_concrete_node<RepeatDate>::type() const       { return NODE_REPEAT_D; }
template<>int ecf_concrete_node<RepeatInteger>::type() const    { return NODE_REPEAT_I; }
template<>int ecf_concrete_node<RepeatDay>::type() const    { return NODE_REPEAT_DAY; }
template<>int ecf_concrete_node<const ecf::LateAttr>::type() const { return NODE_LATE; }
template<>int ecf_concrete_node<ecf::LateAttr>::type() const { return NODE_LATE; }

template<>int ecf_concrete_node<const Event>::type() const    { return NODE_EVENT; }
template<>int ecf_concrete_node<const Label>::type() const    { return NODE_LABEL; }
template<>int ecf_concrete_node<const Meter>::type() const    { return NODE_METER; }

template<>int ecf_concrete_node<const Variable>::type() const    { return NODE_VARIABLE; }
template<>int ecf_concrete_node<ExpressionWrapper>::type() const { return NODE_TRIGGER; }
template<>int ecf_concrete_node<Variable>::type() const { return NODE_VARIABLE;}
template<>int  ecf_concrete_node<const std::pair<std::string, std::string> >
::type() const { return NODE_VARIABLE; }

template<>int ecf_concrete_node<Limit>::type() const { return NODE_LIMIT; }
template<>int ecf_concrete_node<const InLimit>::type() const 
{ return NODE_INLIMIT; }

template<>int ecf_concrete_node<Expression>::type() const { return NODE_TRIGGER; }
template<> int ecf_concrete_node<dummy_node>::type() const { return NODE_UNKNOWN; }
template<> int ecf_concrete_node<dummy_node>::status() const { return STATUS_UNKNOWN; }
template<> const std::string& ecf_concrete_node<dummy_node>::name() const { return owner_->name(); }
template<> const std::string& ecf_concrete_node<dummy_node>::full_name() const
{ return owner_ ? owner_->name() : ecf_node::none(); }
template<> void ecf_concrete_node<dummy_node>::set_graphic_ptr(node* n){}

template<> int ecf_concrete_node<external>::type() const { return NODE_UNKNOWN; }

template<> 
int ecf_concrete_node<Node>::tryno() {
   int num = -1;
   if (owner_) {
      Submittable* submittable = owner_->isSubmittable();
      if ( submittable )
         num = submittable->try_no();
   }
   return num;
}

template<> 
void ecf_concrete_node<Suite>::set_graphic_ptr(node* n) 
{ return owner_->set_graphic_ptr(n); }

template<> 
void ecf_concrete_node<Node>::set_graphic_ptr(node* n)
{ return owner_->set_graphic_ptr(n); }

const std::string& ecf_node::no_owner() { static const std::string NO_OWNER = "(no owner)"; return NO_OWNER; }
const std::string& ecf_node::none()     { static const std::string NONE = "(none)";  return NONE; }
const std::string& ecf_node::slash()    { static const std::string SLASH =  "/";  return SLASH; }

ExpressionWrapper::ExpressionWrapper(Node* n, char c) : 
  node_(n), kind_(c) {
  if (n) {
    if (c=='c')
      mem = n->completeExpression();
    else
      mem = n->triggerExpression();
  }
}

const std::string & ExpressionWrapper::name() const 
{ 
  return ecf_node::none();
}

const std::string & ExpressionWrapper::full_name() const 
{
  return ecf_node::none();
}
const std::string & ExpressionWrapper::toString() const 
{ 
  return mem;
}

void ecf_node::delvars() {
   for (size_t i = 0; i < kids_.size(); i++) {
      if (kids_[i]->type() == NODE_VARIABLE) {
         kids_.erase(kids_.begin() + i);
      }
   }
}

template<>
std::string ecf_concrete_node<Suite>::get_var(const std::string& name, 
                                              bool is_gen,
                                              bool substitute) 
{   
   if (!is_gen) { // user variable have priority
      const Variable& var = owner_->findVariable(name);
      if (!var.empty()) {
         std::string value = var.theValue();
         if (substitute)
            owner_->variableSubsitution(value);
         return value;
      }
   }
   if ((!owner_->repeat().empty()) && name == owner_->repeat().name()) {
      return owner_->repeat().valueAsString();
   }
   return owner_->findGenVariable(name).theValue();
}

template<>
std::string ecf_concrete_node<Node>::get_var(const std::string& name, 
                                             bool is_gen,
                                             bool substitute) 
{
   if (!is_gen) { // user variable have priority
      const Variable& var = owner_->findVariable(name);
      if (!var.empty()) {
         std::string value = var.theValue();
         if (substitute)
            owner_->variableSubsitution(value);
         return value;
      }
   }
   if ((!owner_->repeat().empty()) && name == owner_->repeat().name()) {
      return owner_->repeat().valueAsString();
   }
   return owner_->findGenVariable(name).theValue();
}

template<>
std::string ecf_concrete_node<Defs>::get_var(const std::string& name, 
                                             bool is_gen,
                                             bool substitute) 
{ if (!is_gen) { // user variable have priority
      const Variable& var = owner_->server().findVariable(name);
      if (!var.empty()) {
         std::string value = var.theValue();
         if (substitute)
            owner_->server().variableSubsitution(value);
         return value;
      }
   }
  return owner_->server().findVariable(name).theValue(); 
}

template<>
Limit* ecf_concrete_node<Limit>::get_limit(const std::string& name)
{ return owner_; }
template<>
const Label& ecf_concrete_node<Node>::get_label(const std::string& name)
{ return owner_ ? owner_->find_label(name) : Label::EMPTY(); }
template<>
const Event& ecf_concrete_node<Node>::get_event(const std::string& name)
{ return owner_ ? owner_->findEvent(name) : Event::EMPTY(); }
template<>
const Meter& ecf_concrete_node<Node>::get_meter(const std::string& name)
{ return owner_ ? owner_->findMeter(name) : Meter::EMPTY(); }
template<>
const Repeat& ecf_concrete_node<Node>::get_repeat()
{ return owner_ ? owner_->repeat() : crd(); }

template<>
const Label& ecf_concrete_node<Suite>::get_label(const std::string& name)
{ return owner_ ? owner_->find_label(name) : Label::EMPTY(); }
template<>
const Event& ecf_concrete_node<Suite>::get_event(const std::string& name)
{ return owner_ ? owner_->findEvent(name) : Event::EMPTY(); }
template<>
const Meter& ecf_concrete_node<Suite>::get_meter(const std::string& name)
{ return owner_ ? owner_->findMeter(name) : Meter::EMPTY(); }

template<>
const Repeat& ecf_concrete_node<Suite>::get_repeat()
{ 
  return owner_ ? owner_->repeat() : crd();
}

template<>
Node* ecf_concrete_node<Node>::get_node() const { return owner_; }
template<>
Node* ecf_concrete_node<Alias>::get_node() const { return owner_; }
template<>
Node* ecf_concrete_node<Suite>::get_node() const { return owner_; }
template<>
Node* ecf_concrete_node<Defs>::get_node() const { return 0x0; }

const char* ecf_node_name(int ii) {

  static char* types[] = {
     (char*)"list",    (char*)"user",    (char*)"connection",(char*)"variable",
     (char*)"time",    (char*)"date",    (char*)"trigger",   (char*)"tree",
     (char*)"action",  (char*)"event",   (char*)"task",      (char*)"family",  
     (char*)"suite",   (char*)"super",   (char*)"passwd",    (char*)"login",   
     (char*)"status",  (char*)"reply",   (char*)"check",     (char*)"nid",
     (char*)"file",    (char*)"handle",  (char*)"repeat",    (char*)"dir",  
     (char*)"meter",   (char*)"label",   (char*)"cancel",    (char*)"migrate", 
     (char*)"late",    (char*)"restore", (char*)"complete",  (char*)"nickname",
     (char*)"alias",   (char*)"limit",   (char*)"inlimit",   (char*)"unknown", 
     (char*)"repeat-e", (char*)"repeat-s", (char*)"repeat-d", (char*)"repeat-i", 
     // (char*)"zombie",  (char*)"project", (char*)"object",    (char*)"call",
     (char*)"cron",
      NULL};
  int size = sizeof(types) / sizeof(char*);
  if (ii >= size) return "out of bound";
  return (const char*) types[ii];
}

const std::string ecf_node::type_name() const { return ecf_node_name(type()); }

template<>
void ecf_concrete_node<Defs>::make_subtree() {
  full_name_ = "/";
  if (!owner_) return;

  ChangeMgrSingleton::instance()->attach(owner_, this); 
  make_kids_list(this,owner_->suiteVec());

  std::vector<Variable> gvar = owner_->server().user_variables();
  std::sort(gvar.begin(),gvar.end(),cless_than());
  make_kids_list(this,gvar);

  std::vector<Variable>::const_iterator it;
  gvar = owner_->server().server_variables();
  for (it = gvar.begin(); it != gvar.end(); ++it)
    if (!(*it == Variable::EMPTY()))
      add_kid(make_node(*it, this, 'g'));
    else std::cerr << "# empty variable\n";
}


template<> 
void ecf_concrete_node<Defs>::update(const Defs* n, 
                                     const std::vector<ecf::Aspect::Type>& aspect) 
{
  if (!owner_) return;     
  if (!node_) return;
  if (is_reset(aspect)) {  

     Updating::set_full_redraw();

     XECFDEBUG {
        for (std::vector<suite_ptr>::const_iterator i = n->suiteVec().begin();
                 i != n->suiteVec().end(); ++i) {
           std::cout << "suite name " << (*i)->name() << "\n";
        }}

     return;
  }

  { node_->update(-1, -1, -1); node_->notify_observers(); node_->redraw(); }
}

ecf_node* ecf_node::dummy_node()
{
  ecf_concrete_node<external> *n = new  ecf_concrete_node<external>(0x0, 0x0, 'e');
  return n;
}

