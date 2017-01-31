#ifndef history_H
#define history_H
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


#ifndef panel_H
#include "panel.h"
#endif

#include "timeout.h"
#include "ecflowview.h"
#include "uihistory.h"

class host;

class history : public panel, public history_form_c, public timeout {
public:

	history(panel_window&);

	~history(); // Change to virtual if base class

	virtual const char* name() const { return "History"; }
	virtual void show(node&);
	virtual Boolean enabled(node&);
	virtual void clear();
	virtual Widget widget() { return xd_rootwidget(); }

private:

	history(const history&);
	history& operator=(const history&);

	std::string last_;

	void run();
	void add(host&);
	void update() {}

	virtual void browseCB(Widget,XtPointer);

};

inline void destroy(history**) {}

#endif
