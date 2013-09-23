#ifndef option_H
#define option_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#include "resource.h"

template<class T>
class option : public resource {
public:
	option(configurable*,const str&,const T&);

	~option(); // Change to virtual if base class

	operator const T&() const      { return value_;         }
	const T& operator=(const T& v) { put(v); return value_; }

	virtual void initWidget(Widget);
	virtual bool readWidget(Widget);
	virtual bool changed();

private:

	option(const option<T>&);
	option<T>& operator=(const option<T>&);

	T             value_;

	void put(const T&);
};

#if defined(__GNUC__) || defined(hpux) || defined(AIX)
#include "option.cc"
#endif
#endif
