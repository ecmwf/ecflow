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

/* 
#ifndef lister_H
#include "lister.h"
#endif
*/

template<class T>
T* lister<T>::scan(T* first)
{
	if(sort())
	{
		int swap = 1;

		while(swap)
		{
			T *d = first;
			T *p = 0;
			T *n = d?d->next:0;
			swap = 0;

			while(d && n)
			{
				if(compare(*n,*d))
				{
					T* q = n->next;
				
					n->next = d;
					d->next = q;

					if(p) p->next = n;
					else  first   = n;

					swap++;
					break;
				}
					
				p = d;
				d = n;
				n = n->next;
			}
		}
	}
	T *d = first;
	while(d)
	{
		next(*d);
		d = d->next;
	}
	return first;
}
