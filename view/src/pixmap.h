#ifndef pixmap_H
#define pixmap_H
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

#ifndef extent_H
#include "extent.h"
#endif

#ifndef str_H
#include "str.h"
#endif


class pixmap : public extent<pixmap> {
public:
	pixmap(const char*);
	pixmap(const char*,const char**);
	~pixmap(); // Change to virtual if base class

	Pixmap pixels();
	void set_label(Widget);

	static pixmap& find(const char*);

private:
	pixmap(const pixmap&);
	pixmap& operator=(const pixmap&);

	Pixmap pixmap_;
	str    name_;
	const char** bits_;

	static const char* clean(const char*);
};

inline void destroy(pixmap**) {}
#endif
