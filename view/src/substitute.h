#ifndef substitute_H
#define substitute_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2019 ECMWF.
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

#include <Xm/Xm.h>

#ifndef node_H
#include "node.h"
#endif

class substitute : public extent<substitute> {
public:
  substitute(const std::string&);

  ~substitute(); // Change to virtual if base class
  virtual const std::string eval(node*) = 0;

  static const char* scan(const char*, node*);
  static void fill(Widget);

private:
	substitute(const substitute&);
	substitute& operator=(const substitute&);

	const std::string name_;
};

inline void destroy(substitute**) {}

class proc_substitute : public substitute {
  typedef const std::string& (node::*procc)() const; procc procc_;
  const std::string eval(node* n) { return (n->*procc_)(); }
public:
 proc_substitute(const std::string& name,procc p): substitute(name), procc_(p)
  {}
};
#endif
