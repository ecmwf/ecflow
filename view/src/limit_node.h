#ifndef limit_node_H
#define limit_node_H
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


#include "node.h"
class Limit;

class limit_node : public node {
public:

 limit_node(host& h,ecf_node* n) : node(h,n) {}
#ifdef BRIDGE
 limit_node(host& h,sms_node* n,char b) : node(h,n,b) {}
#endif
  int maximum() const;
  int value() const;
  void nodes(node_lister&);
 protected:
  Limit* get() const;

private:

	limit_node(const limit_node&);
	limit_node& operator=(const limit_node&);

	virtual bool evaluate() const;
	virtual void drawNode(Widget w,XRectangle* r,bool);
	virtual void sizeNode(Widget w,XRectangle* r,bool);
	virtual xmstring make_label_tree();

	virtual void info(std::ostream&);
	virtual const char* status_name() const;
	virtual Boolean visible()  const;
	virtual bool match(const char*);

	virtual void perlify(FILE*);
};

class limit_integer_node : public limit_node {
	void sizeNode(Widget w,XRectangle* r,bool);
	void drawNode(Widget w,XRectangle* r,bool);
	void drawMeter(Widget w,XRectangle* r);
public:
  limit_integer_node(host& h,ecf_node* n): limit_node(h,n) {}
#ifdef BRIDGE
 limit_integer_node(host& h,sms_node* n,char b) : limit_node(h,n,b) {}
#endif
};

class limit_boolean_node : public limit_node {
public:
  limit_boolean_node(host& h,ecf_node* n): limit_node(h,n) {}
#ifdef BRIDGE
  limit_boolean_node(host& h,sms_node* n,char b) : limit_node(h,n,b) {}
#endif    
};

inline void destroy(limit_node**) {}

#endif
