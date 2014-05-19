#ifndef lister_H
#define lister_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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


template<class T>
class lister {
public:
	virtual void next(T&)  = 0;
	virtual bool sort()              { return false; }
	virtual bool compare(T& a,T& b);
	virtual T* scan(T*);
};

template<class T>
bool lister<T>::compare(T& a,T& b)  
{ return strcmp(a.name().c_str(),b.name().c_str()) < 0; }

#if defined(__GNUC__) || defined(hpux) || defined(_AIX)
#include "lister.cc"
#endif

/* 
#ifdef AIX
#pragma implementation("lister.cc") 
#endif
*/

#endif
