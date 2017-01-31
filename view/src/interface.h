#ifndef interface_H
#define interface_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#include <Xm/Xm.h>
#include <string>

class interface {
public:

	interface();

	~interface(); // Change to virtual if base class

	virtual void clear() = 0;
	virtual void message(const char*) = 0;
	virtual void watch(Boolean) = 0;

	virtual void add_host(const std::string&) = 0;
	virtual void remove_host(const std::string&) = 0;
	virtual void rename_host(const std::string&,const std::string&) = 0;

	virtual void login(const char*) = 0;
	virtual void logout(const char*) = 0;

	virtual Widget top_shell() = 0;
	virtual Widget trees() = 0;
	virtual Widget windows() = 0;

	virtual bool visible() { return top_shell() != 0; }
	virtual void error(const char*) = 0;
private:

	interface(const interface&);
	interface& operator=(const interface&);

};

inline void destroy(interface**) {}

#endif
