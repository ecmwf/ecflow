#ifndef relation_H
#define relation_H
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


#ifdef NO_BOOL
#include "bool.h"
#endif

#ifndef extent_H
#include "extent.h"
#endif

class observer;
class observable;
class counted;

class observer_iterator {
public:
	virtual void next(observer*) = 0;
};

class observable_iterator {
public:
	virtual void next(observable*) = 0;
};


class relation : public extent<relation>  {
public:
	relation(observer*,observable*);

	~relation(); // Change to virtual if base class

	static void add(observer*,observable*);
	static int remove(observer*,observable*);
	static int remove(observer*);
	static int remove(observable*);
	static void replace(observable*,observable*);

	static void scan(observer*,  observable_iterator&);
	static void scan(observable*,observer_iterator& );
	static bool gc();


	static void set_data(observer*,observable*,counted*);
	static counted* get_data(observer*,observable*);

	static void stats(const char*);

private:

	relation(const relation&);
	relation& operator=(const relation&);

	observer*   observer_;
	observable* observable_;
	counted*    data_;
	bool        valid_;
};
#endif
