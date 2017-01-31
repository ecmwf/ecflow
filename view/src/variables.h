#ifndef variables_H
#define variables_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #6 $ 
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


#include "uivariables.h"

#ifndef panel_H
#include "panel.h"
#endif

// 

class variables : public panel, public variables_form_c {
public:

	variables(panel_window&);

	~variables(); // Change to virtual if base class

	virtual const char* name() const { return "Variables"; }
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }

private:

	variables(const variables&);
	variables& operator=(const variables&);

	bool loading_;

	virtual void browseCB( Widget, XtPointer ) ;
	virtual void deleteCB( Widget, XtPointer ) ;
	virtual void nameCB( Widget, XtPointer ) ;
	virtual void setCB( Widget, XtPointer ) ;
	virtual void valueCB( Widget, XtPointer ) ;
	virtual void findCB( Widget, XtPointer ) ;

};

inline void destroy(variables**) {}

#endif
