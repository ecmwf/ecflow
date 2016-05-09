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

#include "text_layout.h"
#include "node.h"
#include "trigger_panel.h"
#include "Hyper.h"
#include "tmp_file.h"
#include <stdio.h>
#include "error.h"
#include "trigger_lister.h"

text_layout::text_layout(trigger_panel& t,Widget w):
	layout(t,w)
{
}

text_layout::~text_layout()
{
}

void text_layout::clear()
{
	owner().forget_all();
	HyperSetText(widget_,"");
}

struct info_lister : public trigger_lister {
	panel& p_;
	FILE* f_;
	char* t_;
	Boolean e_;
public:
	info_lister(panel& p,FILE* f,char *t,Boolean e) : p_(p),f_(f), t_(t), e_(e) {}	
	void next_node(node& n, node*,int,node*);
	Boolean parents() { return e_; }
	Boolean kids() { return e_; }
};

void info_lister::next_node(node& n, node* p,int mode,node*)
{
	// Title 
	if(t_) {
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
}


void text_layout::show(node& n)
{
    owner().forget_all();

    tmp_file tmp(tmpnam(0));

    FILE *f = fopen(tmp.c_str(),"w");

    if(!f)
    {
        gui::syserr(tmp.c_str());
        return;
    }

    info_lister i1(owner(),f,"Nodes triggering this node",owner().extended());
    if(owner().triggers())
      n.triggers(i1);
    
    info_lister i2(owner(),f,"Nodes triggered by this node",owner().extended());
    if(owner().triggered())
      n.triggered(i2);
    
    fclose(f);

    HyperLoadFile(widget_,(char*)tmp.c_str());
}

void text_layout::adoption(observable*,observable*)
{
}

void text_layout::gone(observable*)
{
}

void text_layout::notification(observable*)
{
}

void text_layout::reach(node* n1,node* n2)
{
	clear();
	HyperSetText(widget_,"This functionality is only available in graphic mode.");
}
