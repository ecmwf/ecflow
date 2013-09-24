#if defined(AIX_GCC)
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class
//============================================================================

#include <iostream>
#include "File_r.hpp"

using namespace std;

namespace ecf {


File_r::File_r(const std::string& file_name) : file_name_(file_name),
   fp_( fopen(file_name.c_str(),"r") ),
   good_(true)
{
}

File_r::~File_r()
{
   fclose(fp_);
}

bool File_r::ok() const
{
   if (fp_ == 0) return false;
   return true;
}

bool File_r::good() const
{
   if (fp_ == 0) return false;
   return good_;
}

void File_r::getline(std::string& line)
{
   line.erase();
   char buffer[4096] =  { 0 } ;
   int c = 0;
   int i = 0;
   while ( (c = fgetc(fp_)) != EOF) {
      if (c == 10 /* new line */) {
         buffer[i] = '\0';
         break;
      }
      else {
         buffer[i] = c;
      }
      i++;
   }
   if (c == EOF) {
      good_ = false;
   }
   line = buffer;
}

}

#endif
