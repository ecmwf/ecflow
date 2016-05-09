#ifndef DATE_H
#define DATE_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #10 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2016 ECMWF.                                                                  */
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
#
class date_node : public node {

  std::string full_name_;
	virtual bool is_my_parent(node*) const { return false; }
	virtual void info(std::ostream&) {}

	virtual xmstring make_label_tree();
	virtual xmstring make_label_trigger() { return make_label_tree(); }

	virtual const std::string& name()  const;
	const std::string& full_name() const {return full_name_; }
	virtual Boolean menus()       { return False; }
	virtual Boolean selectable() { return False; }

	virtual Boolean visible() const { return show::want(show::date); }
	virtual node* graph_node();
	char* string(char*);

	virtual void perlify(FILE*);

public:
	date_node(host& h,ecf_node* n);
#ifdef BRIDGE
	date_node(host& h,sms_node* n, char b);
#endif
};
#endif
