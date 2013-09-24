#ifndef task_node_H
#define task_node_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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


#include "simple_node.h"

class task_node : public simple_node {
public:
	task_node(host& h,ecf_node* n);
#ifdef BRIDGE
	task_node(host& h,sms_node* n, char b);
#endif
	~task_node(); // Change to virtual if base class

private:
	task_node(const task_node&);
	task_node& operator=(const task_node&);

	virtual void adopt(node*);
	virtual void create();
	virtual void update(int,int,int);
	virtual void check(int,int,int);
	virtual void info(std::ostream&);

	virtual void aborted(std::ostream&);
	virtual void html_output(FILE*,url&);
	virtual void html_script(FILE*,url&);
	virtual void html_job(FILE*,url&);
	virtual void html_jobstatus(FILE*,url&);
	virtual void html_name(FILE*,url&);
	virtual const char* html_page(url&);
};

class alias_node : public task_node {
  void why(std::ostream&);
public:
	alias_node(host& h,ecf_node* n);
#ifdef BRIDGE
	alias_node(host& h,sms_node* n, char b);
#endif
	~alias_node();
};

#endif
