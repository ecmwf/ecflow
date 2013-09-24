#ifndef log_file_H
#define log_file_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


#ifndef input_H
#include "input.h"
#endif

#include "stl.h"


class host;
class log_event;

class log_file : public input {
public:


// -- Exceptions
	// None

// -- Contructors

	log_file(host&,const char*);

// -- Destructor

	~log_file(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators
	// None

// -- Methods

	bool update();

// -- Overridden methods

	

// -- Class members
	// None

// -- Class methods
	// None

	// Uncomment for persistent, remove otherwise
	// static os_typespec* get_os_typespec();

protected:

// -- Members
	// None

// -- Methods
	
	// void print(ostream&) const; // Change to virtual if base class	

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

private:

// No copy allowed

	log_file(const log_file&);
	log_file& operator=(const log_file&);

// -- Members
	 
	 string path_;
	 bool   loading_;
	 bool   ready_;
	 time_t last_;
	 host& owner_;

// -- Methods

	void cleanup();

// -- Overridden methods

	void ready(const char*);
	void done(FILE*);

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const log_file& p)
	//	{ p.print(s); return s; }

};

inline void destroy(log_file**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(log_file);
//#endif

#endif
