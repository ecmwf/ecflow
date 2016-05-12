#ifndef configurable_H
#define configurable_H
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


class configurator;
class resource;

class configurable {
public:
   virtual ~configurable();
	virtual void changed(resource&);

#ifdef alpha
	configurable(const char* name): name_(name) {}
	const char* name() const { return name_; }
private:
	const char* name_;
#else
	virtual const char* name() const = 0;
#endif
};

inline void destroy(configurable**) {}
#endif
