//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "arch.h"
#include "reach.h"
#include "node.h"
#include "array.h"
#include "node_lister.h"
#include "trigger_lister.h"
#include "host.h"

struct path {

	bool  use_;
	bool  mark_;
	bool  ignore_;

	node* from_;
	node* to_;
	node* through_;
	int   mode_;
	node* trigger_;

	path(node* from = 0, node* to = 0,node* through = 0, int mode = 0,node *trigger = 0):
	   use_(false), mark_(false), ignore_(false), from_(from),to_(to),through_(through),mode_(mode), trigger_(trigger)
		{}
	
#ifdef AIX
	bool operator==(const path&) { return 0; }
#endif
};

struct trigger_collector : public trigger_lister {

static array<path> paths_;

	array<node*> nodes_;
	int count_;
	bool triggered_;
	node* current_;

	trigger_collector(node *n): count_(0),triggered_(0),current_(0)  
		{ add(n); add_all(n->kids()); }

	void next_node(node& n,node*,int,node*);
	void add(node*);
	void add_all(node*);
	void add(node*,node*,node*,int,node*);

	void mode(node* c,bool t) { current_ = c; triggered_ = t; }

	Boolean kids() { return True; }
	Boolean parents() { return True; }
};

array<path> trigger_collector::paths_;

void trigger_collector::next_node(node& n,node* th,int mode,node* tr)
{ 
	add(&n);	
	if(triggered_)
		add(current_,&n,th,mode,tr);
	else
		add(&n,current_,th,mode,tr);
}

void trigger_collector::add(node* from, node* to,node* through, int mode,node *trigger)
{
#if 0
	if(through)
	{
		if(from->is_my_parent(through) || through->is_my_parent(from))
			paths_.add( path(from,through,0,trigger_lister::hierarchy,0) );	
		else
			paths_.add( path(from,through,0,mode,trigger) );	

		if(to->is_my_parent(through) || through->is_my_parent(to))
			paths_.add( path(through,to,0,trigger_lister::hierarchy,0) );	
		else
			paths_.add( path(through,to,0,mode,trigger) );	

	}
	else
#endif
		paths_.add( path(from,to,through,mode,trigger) );
}

void trigger_collector::add(node* n)
{
	for(int i = 0; i < nodes_.count(); i++)
		if(nodes_[i] == n)
			return;
	nodes_.add(n);	
}

void trigger_collector::add_all(node* k)
{
	while(k)
	{
		add(k);
		add_all(k->kids());
		k = k->next();
	}
}

/*
static bool sect(trigger_collector& a,trigger_collector& b)
{
	for(int i = 0; i < a.nodes_.count(); i++)
		for(int j = 0; j < b.nodes_.count(); j++)
			if(a.nodes_[i] == b.nodes_[j])
				return true;
	return false;
}
*/
static bool can_reach(node* from,node* to)
{
	bool ok = false;

	if(from == to)
		return true;

	for(int i = 0; i < trigger_collector::paths_.count(); i++)
	{
		path& p = trigger_collector::paths_[i];
		if(!p.ignore_ && !p.mark_ && from == p.from_) {
			if(p.use_) 
				ok = true;
			else
			{
				p.mark_ = true;
				if(can_reach(p.to_,to))
					ok = p.use_ = true; 
				else
					p.ignore_ = true;
				p.mark_ = false;
			}
		}
	}
	return ok;
}

// Not used ?
//static void fill(node* p,node* k)
//{
//	while(k)
//	{
//
//		trigger_collector::paths_.add( path(p,k,0,
//			trigger_lister::hierarchy,0) );
//		trigger_collector::paths_.add( path(k,p,0,
//			trigger_lister::hierarchy,0) );
//		fill(k,k->kids());
//		k = k->next();
//	}
//}

bool reach::join(node* a,node* b,reach_lister& rl)
{

	if(!a || !b) return false;

	trigger_collector::paths_.clear();

	node* top = a->serv().top();
	if(top != b->serv().top())
		return false;

	trigger_collector ta(a);
	trigger_collector tb(b);

	/* while( ! sect(ta,tb) ) */
	for(;;)
	{
		printf("0\n");

		int abefore = ta.count_;
		int bbefore = tb.count_;

		ta.count_ = ta.nodes_.count();
		tb.count_ = tb.nodes_.count();

		int aafter = ta.count_;
		int bafter = tb.count_;
                int i;

		for(i = abefore ; i < aafter; i++)
		{
			ta.mode(ta.nodes_[i],true);
			ta.nodes_[i]->triggered(ta);
		}

		for(i = bbefore ; i < bafter; i++)
		{
			tb.mode(tb.nodes_[i],false);
			tb.nodes_[i]->triggers(tb);
		}

		printf("%d %d %d %d\n",abefore,aafter,bbefore,bafter);

		if(abefore == aafter && bbefore == bafter)
		{
			/* trigger_collector::paths_.clear(); */
			break;
			//return false;
		}
	}


	printf("1\n");
	can_reach(a,b);
	printf("2\n");
#if 0
	for(int i = 0; i < ta.nodes_.count(); i++)
		rl.next(ta.nodes_[i]);

	for(    i = 0; i < tb.nodes_.count(); i++)
		rl.next(tb.nodes_[i]);
#endif

	int x =0;
        int i;
	for(i = 0; i < trigger_collector::paths_.count(); i++)
	{
		path& p = trigger_collector::paths_[i];
		if(p.use_)
			x++;
	}

	printf("paths %d\n",x);

	for(i = 0; i < trigger_collector::paths_.count(); i++)
	{
		path& p = trigger_collector::paths_[i];
		if(p.use_)
			rl.next(p.from_,p.to_,p.through_,p.mode_,p.trigger_);

	}
	trigger_collector::paths_.clear();
	return false;
}


