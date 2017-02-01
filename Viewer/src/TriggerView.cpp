//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerView.hpp"

#include "VNode.hpp"

NodeItem::NodeItem()
{

}

QRectF NodeItem::boundingRect() const
{
    return QRectF();
}

void paint(QPainter*, const QStyleOptionGraphicsItem *,QWidget*)
{

}



TriggerScene::TriggerScene(QWidget *parent) : QGraphicsScene(parent)
{
}

void TriggerScene::reset(VInfo_ptr info)
{
	clear();

}

/*int TriggerScene::grow(VInfo_ptr info,bool)
{

}*/


TriggerView::TriggerView(QWidget *parent) : QGraphicsView(parent)
{

}







////////////////////////////////////////////////////////////////////////
/*

class TriggerListItem
{
public:
    std::string type_;
    std::string name_;
    std::string path_;
protected:

};

class TriggerCollector
{
public:
    TriggerCollector() {}
    virtual ~TriggerCollector() {}

    enum Mode { Normal    = 0,   // Normal trigger_node
                Parent    = 1,   // Through parent
                Child     = 2,   // Through child
                Hierarchy = 3    // Through child
    }

    virtual void add(VItem*, VNode*,Mode,VNode*) = 0;
    virtual bool scanParents() { return false; }
    virtual bool scnaKids()    { return false; }
    virtual bool scanSelf()    { return true; }

private:
    TriggerCollector(const trigger_lister&);
    TriggerCollector& operator=(const trigger_lister&);

    std::vector<VItem*> items_;
};



class TriggerListCollector : public TriggerCollector
{
public:
    TriggerListCollector(FILE* f,const std::string& *title,bool extended) :
        file_(f), title_(t), extended_(extended) {}

    void add(VItem*, VNode*,Mode,VNode*);
    bool scanParents() { return extended_; }
    bool scanKids() { return extended_; }

protected:
    //panel& p_;
    FILE* file_;
    std::string title_;
    bool extended_;
    std::vector<VItem*> items_;
};

void InfoLister::add(VItem* n, VNode* parent,Mode mode,VNode*)
{
    items_.push_back(n);


#if 0
    // Title
    if(title_)
    {
            int n = fprintf(f_,"\n%s:\n",t_) - 2;
            while(n--) fputc('-',f_);
            fputc('\n',f_);
            t_ = 0;
    }

    p_.observe(&n);
    fprintf(f_,"%s {%s}",n.type_name(), n.full_name().c_str());
    if(p) {
        fprintf(f_," through ");
        p_.observe(p);

        switch(mode)
        {
            case trigger_lister::parent:  fprintf(f_,"parent "); break;
            case trigger_lister::child:   fprintf(f_,"child ");  break;
        }

        fprintf(f_,"%s {%s}",p->type_name(),p->full_name().c_str());
    }
    fputc('\n',f_);
#endif
}



class fik : public TriggerList
{
public:
    fik(node* n,node* k,trigger_lister& l): n_(n),k_(k),l_(l) {};

    void next_node(node& n, node*,int type,node *t)
    {
        if(!n.is_my_parent(n_))
        {
        // k is a kid of n whose trigger_panel is outside its subtree
            l_.next_node(n,k_,trigger_lister::child,t);
        }
    }

private:
  node*     n_;
  node*     k_;
  trigger_lister& l_;
};

class fip : public trigger_lister {
  node* p_;
  trigger_lister& l_;

public:
  fip(node* p,trigger_lister& l) : p_(p), l_(l) {}

  void next_node(node& n, node*,int type,node *t)
  { l_.next_node(n,p_,trigger_lister::parent,t); }
};


*/



/*
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
  virtual void visitFunction(AstFunction*){}
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


//Get the triggers list for the Triggers view
void simple_node::triggers(trigger_lister& tlr)
{
	//Check the node itself
	if(tlr.self())
	{
		if(node_ && !node_->isSuite())
		{
			std::set<VNode*> theSet;
			std::set<VNode*>::iterator sit;
			AstCollateXNodesVisitor astVisitor(theSet);

			if(node_->completeAst())
				node_->completeAst()->accept(astVisitor);

			if(node_->triggerAst())
				node_->triggerAst()->accept(astVisitor);

			for (sit = theSet.begin(); sit != theSet.end(); ++sit)
				tlr.next_node( *(*sit), 0, trigger_lister::normal, *sit);
     }


     for(std::vector<VNode*>::itertor it=children_.begin(); it!= n ; n = n->next()) {
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

*/





