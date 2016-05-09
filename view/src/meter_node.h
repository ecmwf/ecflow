#ifndef meter_node_H
#define meter_node_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #11 $ 
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


#ifndef node_H
#include "node.h"
#endif


class meter_node : public node {
public:

  const std::string name_;
  meter_node(host& h,ecf_node* n);
#ifdef BRIDGE
  meter_node(host& h,sms_node* n, char b);
#endif
  int minimum();
  int maximum();
  int value() const;
  int threshold();

protected:

  virtual void perlify(FILE*);

private:

	meter_node(const meter_node&);
	meter_node& operator=(const meter_node&);

	virtual void info(std::ostream&);
	virtual void drawNode(Widget w,XRectangle* r,bool);
	virtual void sizeNode(Widget w,XRectangle* r,bool);
	virtual xmstring make_label_tree();

	virtual Boolean visible() const;
	virtual const char* status_name() const;

	const Meter& get() const;
};

inline void destroy(meter_node**) {}

#endif
