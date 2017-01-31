#ifndef external_H
#define external_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #8 $ 
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

class external : public node {
  std::string name_;
public:
	external(const char*);
	~external(); // Change to virtual if base class

	virtual Boolean menus()       { return False;      }
	virtual Boolean selectable() { return False;      }
	virtual const std::string& full_name()  const  { return name();     }
	virtual void info(std::ostream&);

	virtual const char* type_name() const  { return "external"; }
	virtual const char* status_name() const  { return "unknown"; }
	virtual const std::string& name() const   { return name_; }
	virtual const std::string toString() const   { return name(); }

	static Boolean is_external(const std::string& path) { return is_external(path.c_str()); }
	static Boolean is_external(const char*);
	virtual std::ostream& print(std::ostream&s) const { return s << "extern_node\n";};

	static external& get(const std::string& path) { return get(path.c_str()); }
	static external& get(const char*);

private:

	external(const external&);
	external& operator=(const external&);

	virtual void perlify(FILE*);
};

inline void destroy(external**) {}
#endif
