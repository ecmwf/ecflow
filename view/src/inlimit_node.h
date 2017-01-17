#ifndef INLIMIT_NODE_H
#define INLIMIT_NODE_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #10 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2017 ECMWF.                                                                  */
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

class inlimit_node : public node {

	std::string buf_, full_name_;
	virtual bool match(const char*);

	virtual void info(std::ostream&) {}
	virtual xmstring make_label_tree();

	virtual const std::string& name()   const { return buf_; }
	virtual const std::string& full_name() const;
	virtual Boolean menus()       { return False;      }
	virtual Boolean selectable() { return True;      }

	virtual Boolean visible()  const  { return show::want(show::inlimit);      }

	virtual void triggered(trigger_lister&) {}
	virtual void triggers(trigger_lister&)  {}
	virtual void perlify(FILE*);

public:
  inlimit_node(host& h,ecf_node* n);
#ifdef BRIDGE
  inlimit_node(host& h,sms_node* n, char b); // : node(h,n,b) {}
#endif
  ~inlimit_node();
};

#endif
