#ifndef info_H
#define info_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
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


#include "uiinfo.h"

#ifndef panel_H
#include "panel.h"
#endif

class info : public panel, public info_form_c {
public:
	info(panel_window&);

	~info(); // Change to virtual if base class

	virtual const char* name() const { return "Info"; }
	virtual void show(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }
	virtual Boolean enabled(node&);

private:

	info(const info&);
	info& operator=(const info&);

	virtual void hyperCB(Widget,XtPointer);
};

inline void destroy(info**) {}
#endif
