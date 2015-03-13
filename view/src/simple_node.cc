//===================================================================================(variable_node* run==========
// Name        : 
// Author      : 
// Revision    : $Revision: #53 $ 
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

#include "arch.h"
#include "simple_node.h"
#include "host.h"
#include "show.h"
#include "flags.h"
#include "external.h"
#include "selection.h"
#include "pixmap.h"
#include "url.h"
#include "tree.h"
#include "variable_node.h"
#include "tip.h"

#ifndef FLAG_HPP_
#include "Flag.hpp"
#endif
#include "RepeatAttr.hpp"

#include <X11/X.h>
#include <Xm/ManagerP.h>
#include <Xm/DrawP.h>
#include <sstream>

#include "trigger_lister.h"
#include "text_lister.h"
#include "html_lister.h"

#ifndef events_H
#include "events.h"
#endif
#include "dummy_node.h"

#ifndef ecf_node_
#include "ecf_node.h"
#endif

namespace ecf {
const char *status_name[10]
  = { "unknown", "suspended", "complete", "queued", "submitted", "active",
      "aborted", "shutdown",  "halted"  , NULL };
}

const int kHMargins = 4;
const int kVMargins = 1;

static struct {
  char*   name_;
  int     vers_;
  pixmap* pixmap_;
  flags*  flag_;
  int     show_;
} pix[] = {
  {(char*)"waiting", 0, 0, new procFlag(&node::isWaiting), show::waiting_icon},
  
  {(char*)"clock", 0, 0, new procFlag(&node::hasTimeHolding), show::time_icon},  
  {(char*)"calendar", 0, 0, new procFlag(&node::hasDate), show::date_icon},
  
  {(char*)"late", 0, 0, new procFlag(&node::isLate), show::late_icon},

  {(char*)"rerun", 0, 0, new procFlag(&node::isRerun), show::rerun_icon},

  {(char*)"migrated", 0, 0, new procFlag(&node::isMigrated), 
     show::migrated_icon},

  {(char*)"message", 0, 0, new procFlag(&node::hasMessages), 
   show::message_icon},

  {(char*)"defstatus", 0, 0, new procFlag(&node::isDefComplete), 
   show::defstatus_icon},

  {(char*)"Zbw", 1, 0, new procFlag(&node::hasZombieAttr), show::zombie_icon},

  {(char*)"Z", 1, 0, new procFlag(&node::isZombie), show::zombie_icon},

  {(char*)"force_abort", 1, 0, new procFlag(&node::isForceAbort), 1},
  {(char*)"user_edit", 1, 0, new procFlag(&node::isUserEdit), 1},
  {(char*)"task_aborted", 1, 0, new procFlag(&node::isTaskAbort), 1},
  {(char*)"edit_failed", 1, 0, new procFlag(&node::isEditFailed), 1},
  {(char*)"cmd_failed", 1, 0, new procFlag(&node::isCmdFailed), 1},
  {(char*)"no_script", 1, 0, new procFlag(&node::isScriptMissing), 1},
  {(char*)"killed", 1, 0, new procFlag(&node::isKilled), 1},
  {(char*)"byrule", 1, 0, new procFlag(&node::isByRule), 1},
  {(char*)"queuelimit", 1, 0, new procFlag(&node::isQueueLimit), 1},

  {(char*)"folded", 0, 0, new procFlag(&node::isFolded), 0},

  {(char*)"locked", 0, 0, new procFlag(&node::isLocked), 0}, /* --- shall appear last */

  {(char*)"clock_free", 0, 0, new procFlag(&node::hasTime), show::time_icon},

};

simple_node::simple_node(host& h,ecf_node* n) : node(h,n)
					      , old_status_(-1)
					      , old_tryno_(-1)
					      , old_flags_(-1)
{
   old_flags_ = flags();
   old_status_ = status();
   old_tryno_ = tryno();
}

const int kPixSize = 16;

#ifdef BRIDGE
simple_node::simple_node(host& h,sms_node* n, char b) 
  : node(h,n,b)
  , old_status_(-1)
  , old_tryno_(-1)
  , old_flags_(-1)
{
  insert(node::create(h,(sms_node*)n->label));
  insert(node::create(h,(sms_node*)n->meter));
  insert(node::create(h,(sms_node*)n->event));
  if (n->complete)
    insert(node::create(h,(sms_node*)n->complete,'c'));
  insert(node::create(h,(sms_node*)n->trigger, 't'));
  
  insert(node::create(h,(sms_node*)n->repeat));
  insert(node::create(h,(sms_node*)n->genvars,'g'));
  insert(node::create(h,(sms_node*)n->variable));
  
  insert(node::create(h,(sms_node*)n->limit));
  insert(node::create(h,(sms_node*)n->inlimit));
  
  insert(node::create(h,(sms_node*)n->date));
  insert(node::create(h,(sms_node*)n->time));
  insert(node::create(h,(sms_node*)n->autocm,'c'));
  
  // if(n->late) n->late->parent = owner_;
  insert(node::create(h,(sms_node*)n->late,'l'));
}

/* void simple_node::scan(sms_tree* m,text_lister& f,bool b) {} */

   void simple_node::scan(sms_tree* m,trigger_lister& f,bool b) {} 

#endif

simple_node::~simple_node()
{
}

inline bool wanted(int n)
{
  return (n == 0 || show::want(n));
}

void simple_node::sizeNode(Widget w,XRectangle* r,bool tree)
{
  if(!tree) {
    node::sizeNode(w,r,false);
    return;
  }

  int extra = 0;
  unsigned int i;
  
  if(pix[0].pixmap_ == 0)
    {
      for(i = 0; i < XtNumber(pix); i++)
        pix[i].pixmap_ = &(pixmap::find(pix[i].name_));
    }
  
  for(i = 0; i < XtNumber(pix) ; i++)
    if(wanted(pix[i].show_) && pix[i].flag_->eval(this))
      extra++;
  
  XmString s  = tree?labelTree():labelTrigger();
  XmFontList f = tree?fontlist():smallfont();
  
  r->width    = XmStringWidth(f,s)  + 2*kHMargins +  extra * kPixSize;
  r->height   = XmStringHeight(f,s) + 2*kVMargins;
  if(r->height < kPixSize + 2*kVMargins) r->height = kPixSize + 2*kVMargins;
}

void simple_node::drawBackground(Widget w,XRectangle* r,bool tree)
{
  node::drawBackground(w,r,tree);
  GC gc = colorGC(status());
  XFillRectangles(XtDisplay(w),XtWindow(w),gc,r,1);
}

void simple_node::drawNode(Widget w,XRectangle* r,bool tree)
{
  if(!tree) {
    node::drawNode(w,r,tree);
    shadow(w,r);
    return;
  }

  unsigned int extra = 0;
  Pixmap images[XtNumber(pix)];
  
  XmString s  = tree?labelTree():labelTrigger();
  XmFontList f = tree?fontlist():smallfont();
  unsigned int i;
  
  for(i = 0; i < XtNumber(pix) ; i++)
    if(wanted(pix[i].show_) && pix[i].flag_->eval(this))
      images[extra++] = pix[i].pixmap_->pixels();
  
  XRectangle x = *r;
  x.width      = XmStringWidth(f,s)  + 2*kHMargins;
  XRectangle y = x; // *r;
  
  drawBackground(w,&y,tree);
  
  
  XmStringDraw(XtDisplay(w),XtWindow(w),
               f,
               s,
               blackGC(),
               r->x /*+ kHMargins*/,
               r->y + kVMargins,
               x.width,
               XmALIGNMENT_CENTER, 
               XmSTRING_DIRECTION_L_TO_R, r);
  
  for(i = 0 ; i < extra ; i++)
    {
      /* XSetClipMask(XtDisplay(w),blackGC(),masks[i]); */
      XCopyArea(XtDisplay(w),
                images[i],
                XtWindow(w),
                blackGC(),
                0,0,kPixSize,kPixSize,
                r->x + x.width + (i*kPixSize),
                r->y + (r->height - kPixSize) / 2);
      /* XSetClipMask(XtDisplay(w),blackGC(),None); */
    }
  
  shadow(w,&y);
}


Pixel simple_node::color() const
{
  return colors(status());
}

int simple_node::tryno()  const {
#ifdef BRIDGE 
  if (tree_) return tree_->tryno;
#endif
  return owner_ ? owner_->tryno() : -1; 
}

Boolean simple_node::hasTriggers() const 
{ 
#ifdef BRIDGE 
  if (tree_) return tree_->trigger != 0;
#endif
  return owner_ ? owner_->hasTrigger() : False;
}

Boolean simple_node::hasTime() const /* time is free , yellow background icon */
{ 
#ifdef BRIDGE 
  if (tree_) return tree_->time != 0;
#endif
  if (hasTimeHolding()) return False;
  return owner_ ? owner_->hasTime() : False;
}

Boolean simple_node::hasDate() const 
{ 
#ifdef BRIDGE 
  if (tree_) return tree_->date != 0;
#endif
  return owner_ ? owner_->hasDate() : False;
}

Boolean simple_node::hasTimeHolding() const /* grey */
{
  if (owner_)
    if (owner_->hasTime()) {
      Node *node = owner_->get_node();
      if (!node) return False;
      TimeDepAttrs *attr = node->get_time_dep_attrs();
      if (!attr) return False;
      return !attr->time_today_cron_is_free();
    }
  return False;
}

Boolean simple_node::hasZombieAttr() const  
{ 
#ifdef BRIDGE 
  if (tree_) return ecfFlag(FLAG_ZOMBIE);
#endif
  return owner_ ? owner_->hasZombieAttr() : False;
}

Boolean simple_node::hasManual() const 
{ return True; }

Boolean simple_node::isDefComplete() const {   
#ifdef BRIDGE
  if (tree_) {
    if (tree_->defstatus == STATUS_COMPLETE) 
      return True;
    // else if (tree_ && sms_variable_getvar("SMSNOSCRIPT", tree_))
    //   return True;
    else if (tree_->complete != 0 && 
             (sms_status_trigger(tree_->complete) == FALSE))
      return True;
  }
#endif
  if (!owner_) return False;
  else if (!owner_) 
    return False;
  else if (owner_->defstatus() == STATUS_COMPLETE) 
    return True;
  Node* ecf = __node__() ? __node__()->get_node() : 0;
  if (ecf) {
    AstTop* t = ecf->completeAst();
    if (t)
      if (t->evaluate())
        return True;
  }
  return False;
}

std::string simple_node::variable(const std::string& name, bool substitute)
{
  if (__node__())
    if (__node__()->get_node()) {    
      const Variable & var = __node__()->get_node()->findVariable(name);
      if (!var.empty())  {
        std::string value = var.theValue();
        if (substitute)
          __node__()->get_node()->variableSubsitution(value);
        return value; 
      } // return var.theValue();
    }
  for (node *run = kids(); run; run = run->next()) {
    if (run->type() == NODE_VARIABLE && run->name() == name) {
      variable_node *nvar = (variable_node*) run;
      if (nvar->get_var() != ecf_node::none())
        return nvar->get_var();      
    }
  }
  if (!parent())
    return ecf_node::none();
  return parent()->variable(name, substitute);
}

void simple_node::info(std::ostream& f)
{
  static const std::string inc = "  ";
  node::info(f);
  f << type_name() << " " << name() << "\n";
  {
    if (owner_) {

      if (owner_->type() == NODE_SUITE) {
	Suite* suite = dynamic_cast<Suite*>(owner_->get_node());
	// f << "clock    : "; 
	if (suite->clockAttr()) {
	  suite->clockAttr().get()->print(f); // f << "\n";
	}
      }

      int defs = owner_->defstatus();
      if (defs != STATUS_QUEUED && defs != STATUS_UNKNOWN)
        f << inc << "defstatus " << ecf::status_name[defs] << "\n";

      Node* node = owner_->get_node();
      if (node) {
        // if (node->repeat().toString() != "") // repeat // duplicated on suite node
        //  f << inc << node->repeat().toString() << "\n";

        /* zombies attribute */
        const std::vector<ZombieAttr> & vect = node->zombies();
        std::vector<ZombieAttr>::const_iterator it;
        for (it = vect.begin(); it != vect.end(); ++it)
          f << inc << it->toString() << "\n";

        /* autocancel */
        if (node->hasAutoCancel() && node->get_autocancel())
          f << inc << node->get_autocancel()->toString() << "\n";
      }
    }
    if(status() == STATUS_SUSPENDED)
      f << inc << "# " << type_name() << " " << this->name() << " is " << status_name() 
        << "\n";
    }   
  {
    std::vector<Variable> gvar;
    std::vector<Variable>::const_iterator it, gvar_end;
    ecf_node* prox = __node__();
    if (!prox) return;

         Defs* defs = 0;
         Node* ecf = 0;
         if (dynamic_cast<ecf_concrete_node<Node>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Node>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Task>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Task>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Family>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Family>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Suite>*>(prox)) {
            ecf = dynamic_cast<ecf_concrete_node<Suite>*>(prox)->get();
         }
         else if (dynamic_cast<ecf_concrete_node<Defs>*>(prox)) {
            defs = dynamic_cast<ecf_concrete_node<Defs>*>(prox)->get();
         }
         if (!ecf && !defs) {
	   return;
         }

         if (ecf ) {
            gvar.clear();

	    if (ecf->hasTimeDependencies()) {	      
	      f << inc << "# time-date-dependencies: ";
	      if (ecf->isTimeFree()) f << "free\n"; 
	      else f << "holding\n"; 
	    }
            ecf->gen_variables(gvar);
            for(it = gvar.begin(); it != gvar.end(); ++it) {
	      f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }

            gvar = ecf->variables();
            for(it = gvar.begin(); it != gvar.end(); ++it) {
	      f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }
         }
         if (defs) {
            const std::vector<Variable>& gvar = defs->server().user_variables();
            for(it = gvar.begin(); it != gvar.end(); ++it) {
	      f << inc << "# edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }
            const std::vector<Variable>& var = defs->server().server_variables();
            for(it = var.begin(); it != var.end(); ++it) {
	      f << inc << "edit " << (*it).name() << " '" << (*it).theValue() << "'\n";
            }
         }}

  for (node *run=kids(); run; run=run->next()) 
    if (run->type() == NODE_VARIABLE) {
      /* variable_node *var = dynamic_cast<variable_node*> (run);
      if (var && var->name() == "") { f << inc << "# empty variable!" << "\n"; continue; }
      if (var->isGenVariable(0x0))
        f << inc << "# edit " << run->name() << " '" << var->get_var() << "'" << "\n";
      else 
      f << inc << "edit " << run->name() << " '" << var->get_var() << "'" << "\n";*/
    } else { 
      f << inc;
      int i = run->type();
      if (!owner_ || (i == NODE_SUITE || i == NODE_FAMILY ||
                      i == NODE_TASK  || i == NODE_ALIAS))
        f << run->type_name() << " ";
      f << run->toString() << "\n";
      // f << run->dump() << "\n";
    }
  f << "end" << type_name() << " # " << name() << "\n";
}

void simple_node::scan(Ast *m,trigger_lister& tlr,node* trg)
{
  if(!m) return;
  std::string path = "";
  { AstNode     *an = dynamic_cast<AstNode*>     (m); 
    if(an) { path = an->nodePath(); path = m->expression(); } }
  { AstVariable *an = dynamic_cast<AstVariable*> (m); 
    if(an) { path = an->nodePath(); path = m->expression(); } }

  if (path != "") {
    node* n = parent() ? parent()->find(path) : node::find(path);

    if(n) {
      tlr.next_node(*n,0,trigger_lister::normal,trg);
    } else if (external::is_external(path))
      tlr.next_node(external::get(path),0,trigger_lister::normal,trg);
  } 
  {
    scan(m->left(), tlr,trg);
    scan(m->right(),tlr,trg);
  }
}

/*******************************/

class fik : public trigger_lister {
  node*     n_;
  node*     k_;
  trigger_lister& l_;
public:

  fik(node* n,node* k,trigger_lister& l): n_(n),k_(k),l_(l) {};
  
  void next_node(node& n, node*,int type,node *t) {
    if(!n.is_my_parent(n_)) {
      // k is a kid of n whose trigger_panel is outside its subtree
      l_.next_node(n,k_,trigger_lister::child,t);
    }
  }
};

class fip : public trigger_lister {
  node* p_;
  trigger_lister& l_;

public:
  fip(node* p,trigger_lister& l) : p_(p), l_(l) {}

  void next_node(node& n, node*,int type,node *t) 
  { l_.next_node(n,p_,trigger_lister::parent,t); }
};

/*******************************/

static void find_in_kids(node& n,node* k,trigger_lister& tlr)
{
  while(k) {
    fik f(&n,k,tlr);
    k->triggers(f);
    find_in_kids(n,k->kids(),tlr);
    k = k->next();
  }
}
#include <ExprAstVisitor.hpp>

class AstCollateXNodesVisitor : public ecf::ExprAstVisitor {
public:
  AstCollateXNodesVisitor( std::set<node*>& );
  virtual ~AstCollateXNodesVisitor();
  
  virtual void visitTop(AstTop*){}
  virtual void visitRoot(AstRoot*){}
  virtual void visitAnd(AstAnd*){}
  virtual void visitNot(AstNot*){}
  virtual void visitPlus(AstPlus*){}
  virtual void visitMinus(AstMinus*){}
  virtual void visitDivide(AstDivide*){}
  virtual void visitMultiply(AstMultiply*){}
  virtual void visitModulo(AstModulo*){}
  virtual void visitOr(AstOr*){}
  virtual void visitEqual(AstEqual*){}
  virtual void visitNotEqual(AstNotEqual*){}
  virtual void visitLessEqual(AstLessEqual*){}
  virtual void visitGreaterEqual(AstGreaterEqual*){}
  virtual void visitGreaterThan(AstGreaterThan*){}
  virtual void visitLessThan(AstLessThan*){}
  virtual void visitLeaf(AstLeaf*){}
  virtual void visitInteger(AstInteger*){}
  virtual void visitString(AstString*){}
  virtual void visitNodeState(AstNodeState*){}
  virtual void visitEventState(AstEventState*);
  virtual void visitNode(AstNode*);
  virtual void visitVariable(AstVariable*);
  
private:
  std::set<node*>& theSet_;
};


AstCollateXNodesVisitor::AstCollateXNodesVisitor( std::set<node*>& s) : theSet_(s) {}
AstCollateXNodesVisitor::~AstCollateXNodesVisitor() {}

void AstCollateXNodesVisitor::visitEventState(AstEventState* astNode)
{
}

void AstCollateXNodesVisitor::visitNode(AstNode* astNode)
{
  Node* referencedNode = astNode->referencedNode(); 
  node* xnode = NULL;
  if (referencedNode)
    xnode = (node*) referencedNode->graphic_ptr();
  if ( xnode ) theSet_.insert(xnode);
}

void AstCollateXNodesVisitor::visitVariable(AstVariable* astVar)
{
  Node* referencedNode = astVar->referencedNode();
  if (referencedNode) {
    simple_node* xnode = (simple_node*) referencedNode->graphic_ptr();
    if (0 == xnode) return;

    int type;
    node* run;
    for (run = xnode->kids(); 0 != run; run = run->next()) {
      if (run->name() == astVar->name()) {
        type = run->type();
        if (type == NODE_EVENT 
            || type == NODE_METER 
            || type == NODE_VARIABLE) {
          theSet_.insert(run);
        }
      }
    }
  }
}

void simple_node::triggers(trigger_lister& tlr)
{
  if(tlr.self()) {
#ifdef BRIDGE
    if (tree_) {
      sms_node* ow = tree_;
      if(ow->trigger)  
        scan(((sms_trigger*)ow->trigger)->math,tlr,node::find((sms_node*) ow->trigger));

      if(ow->complete) 
        scan(((sms_trigger*)ow->complete)->math,tlr,node::find((sms_node*) ow->complete));

      sms_limit* x = (sms_limit*) (ow->inlimit);
      while(x)
        {
          node* n = node::find((sms_node*) x->limit);
          if(n) tlr.next_node(*n,0,trigger_lister::normal,n);
          x = x->next;
        }
      
      sms_date* d = ow->date;
      while(d) {
        node* n = node::find((sms_node*) d);
        if(n) tlr.next_node(*n,0,trigger_lister::normal,n);
        d = d->next;
      }
      
      sms_time* t = ow->time;
      while(t) {
        node* n = node::find((sms_node*) t);
        if(n) tlr.next_node(*n,0,trigger_lister::normal,n);
        t = t->next;
      }
    }
    else 
#endif
    if (owner_) {
     if (type() != NODE_SUPER && type() != NODE_SUITE) {
       Node* ecf = __node__() ? __node__()->get_node() : 0;
       std::set<node*> theSet;
       std::set<node*>::iterator sit;
       AstCollateXNodesVisitor astVisitor(theSet);

     if (ecf) {
       if (ecf->completeAst()) {
         ecf->completeAst()->accept(astVisitor);
       }
       if (ecf->triggerAst()) {
         ecf->triggerAst()->accept(astVisitor);
       }
     }
     for (sit = theSet.begin(); sit != theSet.end(); ++sit)
       tlr.next_node( *(*sit), 0, trigger_lister::normal, *sit);
     }
     
     for (node *n = this->kids(); n ; n = n->next()) {
        int type = n->type();
        {
           ecf_concrete_node<InLimit const> *c =
                    dynamic_cast<ecf_concrete_node<InLimit const>*> (n->__node__());
           InLimit const * i = c ? c->get() : 0;
           if (i) {
              node *xn = 0;
              if ((xn = find_limit(i->pathToNode(), i->name())))
                 tlr.next_node(*xn,0,trigger_lister::normal,xn);
           }
        } 
        if (type == NODE_DATE || type == NODE_TIME)
           tlr.next_node(*n,0,trigger_lister::normal,n);
     }
    }
  } 
   if(tlr.parents()) {
     node* p = parent();
     while(p) {
       fip f(p,tlr);
       p->triggers(f);
       p = p->parent();
     }
   }
  
   if(tlr.kids())
      find_in_kids(*this,kids(),tlr);
  }

boost::posix_time::ptime simple_node::status_time() const { 
  if (owner_) return owner_->status_time();
  return boost::posix_time::ptime();
}

int simple_node::flags()  const { 
#ifdef BRIDGE
  if (tree_) return tree_->flags;
#endif
  // FIXME defs
  return owner_ ? owner_->flags() : 0;
}

Boolean simple_node::ecfFlag(int n) const
{  
  return (flags() & (1<<n)) != 0;
}

Boolean simple_node::show_it() const
{
  if(((node*)this) == selection::current_node())
    return True;  
  
  if(show::want(show::time_dependant) && (hasDate() || hasTime()))
    return True;
  
  if(show::want(show::late_nodes) && isLate())
    return True;
  
  if(show::want(show::migrated_nodes) && isMigrated())
     return True;
  
  if(show::want(show::rerun_tasks) && tryno() > 1)
    return True;
  
  if(show::want(show::nodes_with_messages) && hasMessages())
    return True;
  
  if(show::want(show::waiting_nodes) && isWaiting())
    return True;
  
  if(show::want(show::defstatus_icon) && isDefComplete())
    return True;
  
  if(show::want(show::zombie_icon) && isZombie())
    return True;
  
  node* k = kids();
  while(k) {
    if(k->show_it())
      return True;
    k = k->next();
  }
  
  return False;
}

Boolean simple_node::visible() const
{
  int wanted = status() - STATUS_UNKNOWN + show::unknown;
  if((wanted < 32 && (show::want(wanted))) || show::want32(wanted)) 
    return True;

  node* n = kids_;
  while(n) {
    if(n->visible_kid()) return True;
    n = n->next();
  }
  return False;
}

Boolean simple_node::visible_kid() const
{
  return visible();
}


const char* simple_node::status_name() const 
{
#ifdef BRIDGE
  if (tree_) return ecf::status_name[tree_->status];
#endif
  return ecf::status_name[owner_ ? owner_->status() : 0]; 
}

void simple_node::why(std::ostream& f)
{
  if (owner_) 
    owner_->why(f);
  else if(status() == STATUS_SUSPENDED) {
    f << type_name() << " " << this << " is " << status_name() << "\n";
  }
}

void simple_node::scan_limit(Ast* m,std::ostream& f) 
{
  if(!m)  return;
  
  AstNode *an = dynamic_cast<AstNode*> (m);
  if(an) {
    const node* n = node::find(an->nodePath());
    if(!n)
      f << "limit_node not found??\n";
    else if(n->evaluate())
      f << n->type_name() << " " << n->name() << " is " << n->status_name() << "\n";
  } else {
    scan_limit(m->left(),f);
    scan_limit(m->right(),f);
  }
}

int kind(Ast *t) {
  int rc = 0;
  if (!t) return rc;

  ++rc ; if (t->type() == "or")  return rc;
  ++rc ; if (t->type() == "and") return rc;
  ++rc ; if (t->type() == "equal") return rc;
  ++rc ; if (t->type() == "not-equal") return rc;
  ++rc ; if (t->type() == "less-than") return rc;
  ++rc ; if (t->type() == "less-equal") return rc;
  ++rc ; if (t->type() == "greater-than") return rc;
  ++rc ; if (t->type() == "greater-equal") return rc;
  ++rc ; if (t->type() == "plus") return rc;
  ++rc ; if (t->type() == "minus") return rc;
  ++rc ; // if (t->type == "multiply") return rc;
  ++rc ; // if (t->type == "divide") return rc;
  ++rc ; // if (t->type == "mod") return rc;
  ++rc ; // if (t->type == "pow") return rc;
  ++rc ; if (t->type() == "not") return rc;
  ++rc ; if (t->type() == "unary") return rc;
  ++rc ; if (t->type() == "open") return rc;
  ++rc ; if (t->type() == "close") return rc;
  ++rc ; if (t->type() == "node") return rc;
  if (t->type() == "variable") return rc;
  if (t->type() == "event_state") return rc;
  return 0;
}

static struct {
  int eval_;
  int   print_;
  char* pos_;
  char*  neg_;
} names [] = {
 {0,0,(char*)"",(char*)"",},   // M_NIL
 {1,0,(char*)"",(char*)"",},   // M_OR 
 {1,0,(char*)"",(char*)"",},   // M_AND
 {1,1,(char*)"is",                    (char*)"is not",},   // M_EQ 
 {1,1,(char*)"is not",                (char*)"is",},   // M_NE 
 {1,1,(char*)"is less than",          (char*)"is greater or equal to",},   // M_LT
 {1,1,(char*)"is less or equal to",   (char*)"is greater than",},   // M_LE
 {1,1,(char*)"is greater than",       (char*)"is less or equal to",},   // M_GT
 {1,1,(char*)"is greater or equal to",(char*)"is less than",},   // M_GE
 {0,1,(char*)" + ",(char*)" + ",},   // M_ADD
 {0,1,(char*)" - ",(char*)" - ",},   // M_SUB
 {0,0,(char*)"",(char*)"",},   // M_MUL
 {0,0,(char*)"",(char*)"",},   // M_DIV
 {0,0,(char*)"",(char*)"",},   // M_MOD
 {0,0,(char*)"",(char*)"",},   // M_POW
 {0,0,(char*)"",(char*)"",},   // M_NOT
 {0,0,(char*)"",(char*)"",},   // M_UNARY
 {0,0,(char*)"",(char*)"",},   // M_OPEN
 {0,0,(char*)"",(char*)"",},   // M_CLOSE
 {0,0,(char*)"",(char*)"",},   // M_NAME
};

void simple_node::scan(Ast* m,std::ostream& f,bool b) 
{
  if(m == 0) return;
  std::cout << "# scan:" << m->expression() << "\n";
  std::string cp = "";

  { AstNode     *an = dynamic_cast<AstNode*>     (m); 
    if(an) { cp = an->nodePath(); cp = m->expression();} }
  { AstVariable *an = dynamic_cast<AstVariable*> (m); 
    if(an) { cp = an->nodePath(); cp = m->expression();} }
  
  if(cp != "") {
    const node* n = node::find(cp);
    if(!n) {
      if(external::is_external(cp))
        f << " (unknown)";
      else
        f << cp << " (not found?)";
      return;
    }
      
    f << n->type_name() << ' ' << n->name() << '(' << n->status_name() << ')';      
    // node* s = f.source(); if(s && n->is_my_parent(s)) f << cancel;
  } else {
    if(external::is_external(cp))
      f << " (unknown)";
  } 
    
  if(m->type() == "not")
    b = !b;
  
  scan(m->left(),f,b);

  f << ' ' << (b ? names[kind(m)].pos_ : names[kind(m)].neg_) << ' ';
  
  scan(m->right(),f,b);
  
  if(names[kind(m)].print_)
    f << "\n";
}

class why_triggers : public trigger_lister {
  std::ostream& f_;

public:

  virtual Boolean self()      { return False; }
  virtual Boolean kids()      { return True; }
  virtual Boolean parents()   { return True; }
  
  why_triggers(std::ostream& f) : f_(f) {}
  
  void next_node(node& n, node* p,int type,node*) {
    if(p && p->status() == STATUS_QUEUED)
      p->why(f_);
  }
};

void simple_node::tell_me_why(std::ostream& f)
{
  this->why(f); 
  // if (0x0 != parent())    parent()->tell_me_why(f);
}

void simple_node::queued(std::ostream& f)
{
  // Check parents
  node* p = this;
  p->why(f);
  
  // Check for suspended kids
  suspended(f);
  
  // Other triggers
  why_triggers wp(f);
  triggers(wp);
}

void simple_node::aborted(std::ostream& f)
{
  node* k = kids();
  while(k) {
    k->aborted(f);
    k = k->next();
  }
}

void simple_node::suspended(std::ostream& f)
{
  if (type() != NODE_FAMILY && type() != NODE_TASK)
    return;

  if(status() == STATUS_SUSPENDED)
    f << "  # " << type_name() << ' ' << this->name() << " is suspended\n";
  
  node* k = kids();
  while(k) {
    k->suspended(f);
    k = k->next();
  }
}

void simple_node::perlify(FILE* f)
{
  if (node::is_json)
    fprintf(f,"\"kids\": [\n");
  else
    fprintf(f,"kids => [\n");
  
  node* k = kids();
  while(k) {
    k->as_perl(f,!k->isSimpleNode());
    fprintf(f,",\n");
    k = k->next();
  }
  
  if (node::is_json)
    fprintf(f,"{} ],\n");
  else
    fprintf(f,"],\n");
}

Boolean simple_node::isZombie() const
{ 
  return ecfFlag(FLAG_ZOMBIE);
}

Boolean simple_node::isToBeChecked() const
{ 
  int s = status();
  return  s == STATUS_SUSPENDED || s == STATUS_ACTIVE || s == STATUS_SUBMITTED;
}

Boolean suite_node::visible () const { return show_it() ; }

Boolean suite_node::show_it() const {
  if (serv().suites().empty()) 
    return True; // show_all

  std::vector<std::string>::const_iterator it;
  for (it = serv().suites().begin(); it != serv().suites().end(); ++it)
    if (*it == name())
      return simple_node::visible();

  return False;
}

Boolean simple_node::isGenVariable(const char *name) {
 for (node *run = kids(); 0 != run; run = run->next()) {
    if (run->type() == NODE_VARIABLE)
      if (run->name() == name)
        return run->isGenVariable(name);
 }
 return False;
}

void simple_node::genvars(std::vector<Variable>& var) 
{ 
  for (node *run = kids(); run; run = run->next()) {
    if (run->type() == NODE_VARIABLE) {
      if (run->name() == "") std::cerr << "# empty variable!\n";
      else if (run->isGenVariable(0))
        var.push_back(Variable(run->name(), ((variable_node*) run)->get_var(), false/*dont check names*/));
    }
  }
  return;
}

void simple_node::variables(std::vector<Variable>& var) 
{
  for (node *run = kids(); 0 != run; run = run->next()) {
    if (run->type() == NODE_VARIABLE){ 
      if (run->name() == "") std::cerr << "# empty variable!\n";
      else if (!run->isGenVariable(0)) {
        var.push_back(Variable(run->name(), ((variable_node*) run)->get_var(), false/*dont check names*/));
      }
  }
}
}

Boolean simple_node::hasMessages() const
{ 
  if (ecfFlag(FLAG_MESSAGE)) return True;
  // FIXME // cannot call this below while it is called during node redraw
  if (type() == NODE_SUPER) 
    return True;
    // return serv().messages(*this).size() > 0;
  return False; // serv().messages(*this).size() > 0;
}

// void simple_node::unlink() { 
//   if (__node__()) __node__()->unlink(); 
//   for (node *run = kids(); run; run = run->next()) { run->unlink(); }
// }
