#ifndef FILE_R_HPP_
#define FILE_R_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class for file utilities
//============================================================================

#include <boost/noncopyable.hpp>
#include <string>

#if defined(AIX_GCC)
#include <stdio.h>
#else
#include <fstream>
#endif

namespace ecf {

#if defined(AIX_GCC)

class File_r : private boost::noncopyable {
public:

   /// For read only, to work around AIX gcc problems
   File_r(const std::string& file_name);
   ~File_r();
   bool ok() const;
   bool good() const;
   void getline(std::string& line);

   const std::string& file_name() const { return file_name_; }

private:
   std::string file_name_;

   FILE* fp_;
   bool good_;
};

#else

class File_r : private boost::noncopyable {
public:

   File_r(const std::string& file_name) : file_name_(file_name), fp_(file_name.c_str(), std::ios_base::in) {}
   ~File_r() { fp_.close(); }

   bool ok() const { return (fp_) ? true : false; }
   bool good() const { return fp_.good(); }
   void getline(std::string& line) { std::getline(fp_,line); }
   const std::string& file_name() const { return file_name_; }

private:
   std::string file_name_;
   std::ifstream fp_;
};

#endif

}

#endif
