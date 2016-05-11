//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
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

#ifndef url_H
#define url_H

#include <stdio.h>
class tmp_file;
class node;

class text_translator {
public:
   virtual ~text_translator() {}
	virtual void save(FILE*,const char*) = 0;
};

class url_translator: public text_translator {
public:
	virtual void save(FILE*,const char*);
};

class url {
public:

// -- Exceptions
	// None

// -- Contructors

	url(int);

// -- Destructor

	~url(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators

	operator FILE*() { return tmp_; }

// -- Methods

	void process(node*);
	void scan(node*);

	const char* method() const { return method_; }
	const char* what() const { return what_; }

	void not_found() { code_ = 404; }

	void add(tmp_file&);
	void add(tmp_file&,text_translator&);
	void copy(FILE*,FILE*);

// -- Overridden methods
	// None

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

	url(const url&);
	url& operator=(const url&);

// -- Members
	
	char method_[1024];
	char what_[1024];
	int soc_;
	int code_;
	FILE* in_;
	FILE* out_;
	FILE* tmp_;

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

	//friend ostream& operator<<(ostream& s,const url& p)
	//	{ p.print(s); return s; }

};

inline void destroy(url**) {}

// If persistent, uncomment, otherwise remove
//#ifdef _ODI_OSSG_
//OS_MARK_SCHEMA_TYPE(url);
//#endif

#endif
