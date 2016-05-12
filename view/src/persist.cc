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

#include "ecflowview.h"
#include "persist.h"

persist::persist(const char* kind,const char* name):
	kind_(kind),
	name_(name),
	f_(0),
	write_(0)
{
}

persist::~persist()
{
	close();
}

void persist::close()
{
	if(f_) {
		if(fclose(f_))
			perror("persist::~persist");
	}
}

bool persist::open(bool w)
{
	if(w != write_ || f_ == 0)
	{
		close();

		char buf[1024];
		const char * rcdir = getenv("ECFLOWRC") ? getenv("ECFLOWRC") : "ecflowrc";

		sprintf(buf,"%s/.%s/%s.%s",rcdir, getenv("HOME"),kind_,name_);

		f_  = fopen(buf, w ? "w" : "r");
		if(!f_) perror(buf);

		write_ = w;

	}
	return f_ != 0;
}


void persist::set(const char* p,int n)
{
	if(!open(true)) return;
	fprintf(f_,"%s: %d\n",p,n);
}

void persist::set(const char* p,const char* s)
{
	if(!open(true)) return;
	fprintf(f_,"%s: %s\n",p,s);
}

/* 
void persist::set(const char* p,sms_list* l)
{
	if(!open(true)) return;

	while(l) 
	{
		fprintf(f_,"%s: %s\n",p,l->name);
		l = l->next;
	}
}

bool persist::get(const char* p,sms_list*& l)
{
	if(!open(false)) return false;
	rewind(f_);

	l = 0;
	const char* x;
	while(( x = read(p)))
	{
		sms_list *m = (sms_list*)ecf_node_create((char*)x);
		m->next = l;
		l = m;
	}

	return l != 0;

}
*/

bool persist::get(const char* p,int& n)
{
	if(!open(false)) return false;
	rewind(f_);

	const char* x;

	if(( x = read(p)))
	{
		n = atoi(x);
		return true;
	}
	return false;
}

bool persist::get(const char* p, char* n)
{
	if(!open(false)) return false;
	rewind(f_);
	const char* x;
	if(( x = read(p)))
	{
		strcpy(n,x);
		return true;
	}
	return false;
}

const char* persist::read(const char* p)
{

	static char line[1024];
	int len = strlen(p);

	for(;;)
	{
		if(!fgets(line,sizeof(line),f_))
			return 0;

		line[strlen(line)-1] = 0;

		if(line[len] == ':' && strncmp(line,p,len) == 0)
			return line + len + 2;
	
	}
}
