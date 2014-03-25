#ifndef selection_H
#define selection_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#ifndef extent_H
#include "extent.h"
#endif
#include <string>

class host;
class node;
class ecf_node;

class selection : public extent<selection> {
public:

	selection();

	virtual ~selection(); // Change to virtual if base class

	virtual void selection_cleared()  = 0;
	virtual void new_selection(node&) = 0;

	static void notify_new_selection(node*);
	static void notify_selection_cleared();

	static node* menu_node();
	static void menu_node(node*);
	static node* current_node();
	static const std::string current_path();

private:

	selection(const selection&);
	selection& operator=(const selection&);

	friend class selection_observer;
};

inline void destroy(selection**) {}
#endif
