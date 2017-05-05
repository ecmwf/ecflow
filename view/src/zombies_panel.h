#ifndef zombies_panel_H
#define zombies_panel_H

#include "uizombies.h"

#ifndef panel_H
#include "panel.h"
#endif

#include <set>

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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

class zombies_panel : public panel, public zombies_form_c {
public:

	zombies_panel(panel_window&);

	~zombies_panel(); // Change to virtual if base class

	virtual const char* name() const { return "Zombies"; }
	virtual void show(node&);
	virtual void clear();
	virtual Boolean enabled(node&);
	virtual Widget widget() { return zombies_form_c::xd_rootwidget(); }

	virtual void create (Widget parent, char *widget_name = NULL);

private:

	zombies_panel(const zombies_panel&);
	zombies_panel& operator=(const zombies_panel&);

        char* name_;

        void call(int, XtPointer);

        virtual void browseCB( Widget, XtPointer );
        virtual void deleteCB( Widget, XtPointer );
	virtual void acceptCB( Widget, XtPointer );
        virtual void rescueCB( Widget, XtPointer );
        virtual void terminateCB( Widget, XtPointer );
        virtual void killCB( Widget, XtPointer );

        std::set<std::string> selection_;
};

inline void destroy(zombies_panel**) {}
#endif
