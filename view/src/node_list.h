#ifndef node_list_H
#define node_list_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


#ifdef NO_BOOL
#include "bool.h"
#endif

class node;

#include <Xm/Xm.h>
#ifndef observer_H
#include "observer.h"
#endif

class node_list : public observer {
public:

	node_list();

	~node_list(); // Change to virtual if base class

	virtual Widget list() = 0;
	virtual Widget form() = 0;
	virtual bool keep(node*) = 0;

	virtual void add(node* n,bool sel = false);
	virtual void remove(node* n);
	virtual void reset();

protected:

	node* find(XmString);
	node* find(const char*);
	const char* name(node*);

private:

	node_list(const node_list&);
	node_list& operator=(const node_list&);

	virtual void notification(observable*);
	virtual void adoption(observable*,observable*);
	virtual void gone(observable*);
};

inline void destroy(node_list**) {}
#endif
