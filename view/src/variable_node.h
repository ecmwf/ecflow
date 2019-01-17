#ifndef VARIABLE_NODE
#define VARIABLE_NODE
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #12 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2019 ECMWF.                                                                  */
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
#include "host.h"
#include "node_editor.h"

class variable_node : public node {

	bool generated_;

	virtual void info(std::ostream&);
	virtual const xmstring& labelTree();
	virtual xmstring make_label_tree();
	virtual void drawNode(Widget w,XRectangle *r,bool);

	virtual Boolean visible() const;
	virtual bool match(const char*);

	virtual void edit(node_editor&);
	virtual void apply(node_editor&);

	virtual void triggered(trigger_lister&) {}
	virtual void triggers(trigger_lister&)  {}

	virtual void perlify(FILE*);

public:
	variable_node(host& h,ecf_node* n);
	Boolean isGenVariable(const char *name) { return generated_; }
	std::string get_var(bool subsitute=false);
#ifdef BRIDGE
	variable_node(host& h,sms_node* n, char b);
#endif

};

#endif
