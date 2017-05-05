#ifndef array_H
#include "array.h"
#endif
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

template<class T>
array<T>::array():
	count_(0),
	max_(0),
	values_(0)
{
}

template<class T>
array<T>::array(const array<T>& other):
	count_(other.count_),
	max_(other.count_),
	values_(0)
{
	values_ = new T[count_];
	for(int i = 0; i< count_; i++)
		values_[i] = other.values_[i];
}

template<class T>
array<T>::~array()
{
	delete[] values_;
}

template<class T>
void array<T>::clear()
{
	count_ =  0; 
}

template<class T>
void array<T>::add(const T& t)
{
	//printf("1. array<T>::add( %d  %d\n", max_,count_);
	if(count_ == max_)
	{
		max_ += 1 + max_/2;
		T* o = new T[max_];
		for(int i = 0; i< count_; i++)
			o[i] = values_[i];
		delete[] values_;
		values_ = o;
		//printf("2. array<T>::add( %d  %d\n", max_,count_);
	}
	//printf("3. array<T>::add( %d  %d\n", max_,count_);
	values_[count_++] = t;
}

template<class T>
void array<T>::remove(const T& t)
{
	for(int i = 0; i< count_; i++)
	{
		if(values_[i] == t)
		{
			values_[i] = values_[--count_];
			break;
		}
	}
}
