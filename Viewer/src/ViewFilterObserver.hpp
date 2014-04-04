//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VIEWFILTEROBSERVER_HPP_
#define VIEWFILTEROBSERVER_HPP_

class ViewFilterObserver
{
public:
	ViewFilterObserver(){};
	virtual ~ViewFilterObserver(){};
	virtual void notifyFilterChanged()=0;
};


#endif
