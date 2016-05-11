#ifndef resource_H
#define resource_H

//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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

#ifndef configurable_H
#include "configurable.h"
#endif

#ifndef str_H
#include "str.h"
#endif

#include <Xm/Xm.h>

class base;

#ifndef resource_H
#include "resource.h"
#endif
#include "extent.h"

class resource : public extent<resource> {
public:

	resource(configurable*,const str&,const str&);

	virtual ~resource(); // Change to virtual if base class

	const str name() const { return name_; }

	virtual void initWidget(Widget) = 0;
	virtual bool readWidget(Widget) = 0;
	virtual bool changed() = 0;

	virtual void       set(const str&);
	virtual const str& get();

	virtual bool isSet() { return set_;  }
	virtual void setset(bool);

	static void init(configurable&,configurator&);
	static void modified(configurable&,configurator&);

private:

	resource(const resource&);
	resource& operator=(const resource&);

	configurable* owner_;
	base*         base_;
	str           name_;
	str           value_;
	bool          set_;
};

inline void destroy(resource**) {}
#endif
