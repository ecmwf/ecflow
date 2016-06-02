#ifndef label_node_H
#define label_node_H

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #9 $ 
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

#include "node.h"

class label_node : public node {
public:

 label_node(host& h,ecf_node* n) : node(h,n) {}
#ifdef BRIDGE
 label_node(host& h,sms_node* n, char b) : node(h,n,b) {}
#endif
	const char* value();
	const char* def();

protected:

private:

	label_node(const label_node&);
	label_node& operator=(const label_node&);

	const Label& get();

	virtual void info(std::ostream&);
	virtual xmstring make_label_tree();
	virtual void drawNode(Widget,XRectangle*,bool);

	virtual Boolean visible() const;
	virtual void triggered(trigger_lister&) {}
	virtual void triggers(trigger_lister&)  {}

	virtual void perlify(FILE*);
};

inline void destroy(label_node**) {}

#endif
