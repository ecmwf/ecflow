#ifndef dummy_node_H
#define dummy_node_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #11 $ 
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

class dummy_node : public node {
public:

  dummy_node(const std::string);
  ~dummy_node(); // Change to virtual if base class

  virtual Boolean menus()      { return False;      }
  virtual Boolean selectable() { return False;      }
  virtual const std::string& full_name()  const  { return name();}
  virtual const std::string& name()  const  { return name_; }
  virtual void info(std::ostream&);
  
  const std::string toString() const;
  virtual const char* type_name() const  { return "dummy_node"; }
  virtual const char* status_name() const  { return "unknown"; }
  virtual std::ostream& print(std::ostream&s) const { return s << "dummy_node\n";};
   
  static dummy_node& get(const std::string);
private:

  dummy_node(const dummy_node&);
  dummy_node& operator=(const dummy_node&);
  
  const std::string name_;
  // int  type_;
  int type() const { return NODE_UNKNOWN; } 
  int  status_;
  virtual void perlify(FILE*); 
};

inline void destroy(dummy_node**) {}

#endif
