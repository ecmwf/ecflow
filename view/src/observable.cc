//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#ifndef observable_H
#include "observable.h"
#endif

#ifndef relation_H
#include "relation.h"
#endif

#include "observer.h"

observable::observable()
 : observed_(false)
{
}

struct gone_iter : public observer_iterator {
	observable* o_;
	void next(observer* o) { o->gone(o_); }
public:
	gone_iter(observable* o) : o_(o) {}
};

observable::~observable()
{
	if(observed_) {
		gone_iter gi(this);
		relation::scan(this,gi);
		relation::remove(this);
	}
}


struct notify_iter : public observer_iterator {
	observable* o_;
	void next(observer* o) { o->notification(o_); }
public:
	notify_iter(observable* o) : o_(o) {}
};

void observable::notify_observers()
{
	if(observed_) {
		notify_iter ni(this);
		relation::scan(this,ni);
	}
}

struct adopt_iter : public observer_iterator {
	observable* o_;
	observable* n_;
	void next(observer* o) { o->adoption(o_,n_); }
public:
	adopt_iter(observable* o,observable* n) : o_(o), n_(n) {}
};

void observable::notify_adoption(observable* n) 
{
	if(observed_ && n) {
		adopt_iter ai(this,n);
		relation::scan(this,ai);
		relation::replace(this,n);
		n->observed_ = true;
	}
}
