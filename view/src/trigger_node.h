#ifndef TRIGGER_NODE_H
#define TRIGGER_NODE_H
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #13 $                                                                    */
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

#ifndef ecf_node_
#include "ecf_node.h"
#endif

class trigger_node : public node {
  std::string expression_;
  std::string full_name_;
  bool complete_;

  const AstTop* get() const;
  
  virtual xmstring make_label_tree();
  void drawNode(Widget w,XRectangle* r,bool);

  virtual const std::string& name() const;
  virtual const std::string& full_name() const { return full_name_; }
  virtual const std::string& definition() const { return expression_; }
  virtual Boolean menus() { return False; }
  virtual Boolean selectable() { return True; }

  virtual Boolean visible() const { return show::want(show::trigger); }

  virtual void triggered(trigger_lister&) {}
  virtual void triggers(trigger_lister&)  {}
  virtual void perlify(FILE*);

public:
  trigger_node(host& h,ecf_node* n);
  ~trigger_node() {}

  virtual void info(std::ostream&);

#ifdef BRIDGE
  virtual bool match(const char*);
  trigger_node(host& h,sms_node* n, char b);
#endif
};
#endif
