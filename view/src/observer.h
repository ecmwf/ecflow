#ifndef observer_H
#define observer_H
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


class observable;
class relation_data;
 
class observer {
public:

	observer();

	virtual ~observer(); // Change to virtual if base class

	void observe(observable*);
	int forget(observable*);
	void forget_all();

	virtual void notification(observable*)          = 0;
	virtual void adoption(observable*,observable*) = 0;
	virtual void gone(observable*)                 = 0;

	void set_data(observable*,relation_data*);
	relation_data* get_data(observable*);

private:

	observer(const observer&);
	observer& operator=(const observer&);
};

/* #include "observer.cc" */

#endif
