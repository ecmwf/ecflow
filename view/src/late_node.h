#ifndef LATE_NODE_H
#define LATE_NODE_H

/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #11 $                                                                    */
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

class late_node : public node {

	virtual bool is_my_parent(node*) const { return false; }
	virtual void info(std::ostream&) {}

	virtual xmstring make_label_tree();

	virtual const std::string& name()  const;
	virtual const std::string& full_name() const    { return name(); }
	virtual Boolean menus()       { return False;      }
	virtual Boolean selectable() { return False;      }

	virtual Boolean visible() const { 
          return show::want(show::late); }

	virtual void perlify(FILE* f);

public:
 late_node(host& h,ecf_node* n) : node(h,n), label_(n ? n->toString() : "late") {}
#ifdef BRIDGE
	late_node(host& h,sms_node* n, char b);
#endif
 
protected:
	const std::string label_;
};

#endif
