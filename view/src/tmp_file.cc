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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tmp_file.h"
#include <iostream>
#include <fstream>
#include <ecflowview.h>

tmp_file_imp::tmp_file_imp(const char* p,bool del):
	str_(p ? strdup(p) : 0),
	del_(del)
{
}

tmp_file_imp::~tmp_file_imp()
{
  if(str_) {
    if(del_) { 
      unlink(str_);
    }
    free(str_);
  }
}

const char* tmp_file_imp::str()
{
	return str_;
}

tmp_file::tmp_file(const char* p,bool del):
	imp_(new tmp_file_imp(p,del))
{
	imp_->attach();
}

tmp_file::tmp_file(const std::string& p,bool del):
  imp_(new tmp_file_imp(tmpnam((char*)tmpName),del)) 
{
  imp_->attach();
  std::ofstream f (imp_->str());
  if (f.is_open())
    { 
      f << p; 
      f.close(); 
    }
}

const char* tmp_file::c_str() { 
    if (imp_) return imp_->str(); 
    return 0x0;
}

tmp_file::~tmp_file()
{
  if(imp_) 
    imp_->detach();
}

tmp_file::tmp_file(const tmp_file& other):
	imp_(other.imp_)
{
	imp_->attach();
}

tmp_file& tmp_file::operator=(const tmp_file& other)
{
  if (other.imp_)
    other.imp_->attach();
  if (imp_) {
    imp_->detach();
  }
  imp_ = other.imp_;
  return *this;
}

#if defined(linux) || defined(_AIX)

char * tmpnam(char *) __THROW {
  char *path = getenv("SCRATCH");
  char *s = (char*) malloc(128);
  if (!path || !access(path, R_OK))
    path=getenv("TMPDIR");
  if (!path || !access(path, R_OK))
    path=(char*)"/tmp";
  snprintf(s, 128, "%s/%sXXXXXX", path, appName);
  // int fid = 
    mkstemp(s);
  // return s = tempnam(path, appName);
  return s;
}

#endif

