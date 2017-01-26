#ifndef hyper_lister_H
#define hyper_lister_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


#include "node.h"
#include "panel.h"
#include "text_lister.h"


class hyper_lister : public text_lister {
public:

  hyper_lister(panel&,node*);
  
  virtual ~hyper_lister(); // Change to virtual if base class
  
  virtual void line(const char*) = 0;
  
  void push(node* n);
  void push(const char* p,...);
  void endline();
  void cancel();
  node* source() const { return node_; }
        
private:
  static const int dim_;

  hyper_lister(const hyper_lister&);
  hyper_lister& operator=(const hyper_lister&);
  
  panel& owner_;
  node*  node_;
  int    nodes_;
  int    cancels_;
  char   buf_[1024];

 protected:
  panel& owner() { return owner_; }
};

inline void destroy(hyper_lister**) {}

#endif
