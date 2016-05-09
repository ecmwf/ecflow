#ifndef extent_H
#define extent_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


template<class T>
class extent {
public:
	extent();

	virtual ~extent(); // Change to virtual if base class

	static void delete_all();

	extent<T>* next_;
	extent<T>* prev_;

	static extent<T>* first_;
	static extent<T>* last_;

	static T* first() { return (T*)first_; }
	T* next()         { return (T*)next_; }
};

template<class T> extent<T>* extent<T>::first_ = 0;
template<class T> extent<T>* extent<T>::last_  = 0;

template<class T>
extent<T>::extent():
	next_(0),
	prev_(last_)
{
	if(last_) 
		last_->next_ = this;
	else 
		first_ = this;
	last_ = this;
}

template<class T>
extent<T>::~extent()
{
	if(prev_) prev_->next_ = next_; else first_ = next_;
	if(next_) next_->prev_ = prev_; else last_  = prev_;
}


template<class T>
void extent<T>::delete_all()
{
	while(first_) delete first_;
}

// gcc is broken
#if defined (__GNUC__) || defined (hpux) || defined(mips) || defined(_AIX)
#define IMP(T)
#else
#define IMP(T) extent<T>* extent<T>::first_;extent<T>* extent<T>::last_;
#endif

#endif
