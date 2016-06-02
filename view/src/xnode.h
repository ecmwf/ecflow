#ifndef xnode_H
#define xnode_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#include <Xm/Xm.h>
extern "C" {
#include "SimpleBase.h"
}

class node;

class xnode {
public:
  xnode(node*);

  virtual ~xnode(); // Change to virtual if base class

  node* get_node() { return node_; }

  virtual int getBox(Widget) ;

  virtual void draw(Widget,XRectangle*) = 0;
  virtual void size(Widget,XRectangle*) = 0;

  void select();

  Widget widget() { return widget_; }

  Boolean visibility(Boolean vis) 
    { return NodeVisibility(widget_,box_,vis); }

  void relation(xnode* o)
    { NodeAddRelation(widget_,box_,o->box_); }

  void* relation_data(xnode* o)
    { return NodeGetRelationData(widget_,box_,o->box_); }

  void* relation_data(xnode* o,void *d)
    { return NodeSetRelationData(widget_,box_,o->box_,d); }

  GC relation_gc(xnode* o,GC gc)
    { return NodeSetRelationGC(widget_,box_,o->box_,gc); }

  void redraw() { NodeNewSize(widget_,box_);NodeChanged(widget_,box_); }
  void show()   { NodeShow(widget_,box_); }

  void setFocus() { NodeSetFocus(widget_,box_); }

  int group()       { return NodeGetGroup(widget_,box_); }
  void group(int g) { NodeSetGroup(widget_,box_,g); }

  static void drawCB(Widget,XRectangle*,XtPointer);
  static void sizeCB(Widget,XRectangle*,XtPointer);

  void *operator new(size_t n) { return XtMalloc(n); }
  void  operator delete(void* d) { XtFree((char*)d); }

protected:

  Widget widget_;
  node*  node_;
  int    box_;

private:

  xnode(const xnode&);
  xnode& operator=(const xnode&);
};

inline void destroy(xnode**) {}
#endif
