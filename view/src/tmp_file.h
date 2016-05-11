#ifndef tmp_file_H
#define tmp_file_H
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


#ifdef NO_BOOL
#include "bool.h"
#endif

#ifndef counted_H
#include "counted.h"
#endif

#include <string>

/** char * tmpnam(s)	char *s;
 *  enable P_tmpdir local definition to change default /tmp directory
 *  enable using new location up to 44 char
 */

#ifndef __THROW
#define __THROW
#endif
#ifdef linux 

extern "C" {
  extern char * tempnam(const char *d, const char* s) __THROW;
}

#ifndef tmpnam1
#include <stdlib.h>
#define tmpnam1 
char * tmpnam(char *) __THROW;
#endif

#endif

class tmp_file_imp : public counted {
	char* str_;
	bool  del_;
public:
	tmp_file_imp(const char*,bool);
	~tmp_file_imp();
	const char* str();
};

class tmp_file {
 public:
  
  tmp_file(const char* str,bool = true);
  
  // str input IS the file content, not filename
  tmp_file(const std::string& str,bool = true);
  tmp_file(const tmp_file&);
  
  ~tmp_file();
  
  tmp_file& operator=(const tmp_file&);
  
  const char* c_str(); /* { if (imp_) return imp_->str(); 
			  return load.c_str();} */

private:

  tmp_file_imp* imp_;
};

inline void destroy(tmp_file**) {}

#endif
