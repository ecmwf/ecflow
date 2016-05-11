#ifndef repeat_node_H
#define repeat_node_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #18 $ 
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
typedef char str80[80];
#include <RepeatAttr.hpp>
class repeat_node : public node {
public:
  // const Repeat& get();
  RepeatBase* get() const;

  repeat_node(host& h,ecf_node* n);
#ifdef BRIDGE
  repeat_node(host& h,sms_node* n,char b);
#endif
  virtual int  start() const;
  virtual int  last() const;
  virtual int  step() const;
  virtual int  current() const;
  virtual void value(char*,int) const;
  virtual int  index(const char* p) const { return atol(p); }   
  virtual bool can_use_text()             { return true; }
  virtual void perlify(FILE*);

  virtual void drawNode(Widget w,XRectangle* r,bool);
  virtual void sizeNode(Widget w,XRectangle* r,bool);
  
 protected:
  static const int ink = 1;
private:
  std::string name_,full_name_;
  
  repeat_node(const repeat_node&);
  repeat_node& operator=(const repeat_node&);
  
  virtual void info(std::ostream&);
  virtual xmstring make_label_tree();
  
  virtual const char* status_name() const;

  virtual const std::string& full_name() const { return full_name_; }
  virtual const std::string& name() const { return name_; }

  virtual bool is_my_parent(node*) const { return false; }
  virtual Boolean visible() const;
};

inline void destroy(repeat_node**) {}
#endif
