#ifndef pref_editor_H
#define pref_editor_H
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


#include <Xm/Xm.h>

#ifndef option_H
#include "option.h"
#endif

#ifndef configurator_H
#include "configurator.h"
#endif

#ifndef editor_H
#include "editor.h"
#endif

class configurable;

class pref_editor : public configurator, public editor {
public:

	virtual configurable* owner() = 0;

	virtual void init(resource&);
	virtual bool modified(resource&);

protected:
	
	void changed(Widget);
	void use(Widget);

private:

	void sensitive(Widget,const char*,bool);
	Widget toggle(const char* n);
};

inline void destroy(pref_editor**) {}
#endif
