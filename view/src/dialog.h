#ifndef dialog_H
#define dialog_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#ifndef singleton_H
#include "singleton.h"
#endif

template<class T,class U>
class dialog : public U, public singleton<T>  {
public:

	dialog() : stop_(True), ok_(False) {}

protected:

	Boolean stop_;
	Boolean ok_;

	Boolean modal(const char*,Boolean);
	void show();

private:

	dialog(const dialog<T,U>&);
	dialog<T,U>& operator=(const dialog<T,U>&);

	virtual void helpCB( Widget, XtPointer ) ;
	virtual void cancelCB( Widget, XtPointer ) ;
	virtual void okCB( Widget, XtPointer ) ;
};

#if defined(__GNUC__) || defined(hpux) || defined(_AIX)
#include "dialog.cc"
#endif
#endif
