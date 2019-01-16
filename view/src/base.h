#ifndef base_H
#define base_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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

#ifndef str_H
#include "str.h"
#endif

#ifndef runnable_H
#include "runnable.h"
#endif

struct pairs;

class base : public extent<base>, public runnable {
public:
	base(const str&,const str&,bool,base*);

	~base(); // Change to virtual if base class

	void attach();
	void detach();

	void defaults(const str&,const str&);
	bool fetch(const str&,str&);
	void store(const str&,const str&,bool);
	void remove(const str&);
	void save();

	static base* lookup(const str&);

private:

	base(const base&);
	base& operator=(const base&);
	virtual void run();

	str    name_;
	str    dir_;
	int    count_;
	pairs* pairs_;
	base*  parent_;
	bool   save_;
};

inline void destroy(base**) {}
#endif
