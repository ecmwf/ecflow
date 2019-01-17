#ifndef globals_H
#define globals_H
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


#ifndef option_H
#include "option.h"
#endif

#ifndef configurable_H
#include "configurable.h"
#endif


class globals : public configurable {
public:

	globals();

	~globals(); // Change to virtual if base class

	static globals* instance();

	static void set_resource(const str&,int);
	static int  get_resource(const str&,int);

	static int  user_level();

private:

	globals(const globals&);
	globals& operator=(const globals&);

	virtual const char* name() const;
	virtual void changed(resource&);
};

inline void destroy(globals**) {}

#endif
