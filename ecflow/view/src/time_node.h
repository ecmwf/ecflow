#ifndef TIME_H
#define TIME_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #10 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2012 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/
#include "node.h"
#include "show.h"
#include "dummy_node.h"

class time_node : public node {
  std::string time_, full_name_;
	virtual node* graph_node();

	virtual bool is_my_parent(node*) const { return false; }
	virtual void info(std::ostream&) {}

	virtual xmstring make_label_tree();
	virtual xmstring make_label_trigger() {
		return make_label_tree();
	}

	virtual const std::string& name()  const        { return time_; }
	const std::string& full_name() const { return full_name_; }
	virtual Boolean menus()       { return False;      }
	virtual Boolean selectable() { return False;      }

	virtual Boolean visible() const { return show::want(show::time); }

	char* string(char*);
	virtual void perlify(FILE*);

public:
	time_node(host& h,ecf_node* n);
#ifdef BRIDGE
	time_node(host& h,sms_node* n, char b);
#endif
};
#endif
