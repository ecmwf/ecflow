#ifndef INDENTOR_HPP_
#define INDENTOR_HPP_
//============================================================================
// Name        : Indentor
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class to provide indented output
//============================================================================
#include <ostream>

namespace ecf {

class Indentor {
public:
   Indentor() {
      ++index_;
   }
   ~Indentor() {
      --index_;
   }

   static std::ostream& indent( std::ostream& os , int char_spaces = 2);

private:
   static int index_;

private:
   friend class DisableIndentor;
   static void disable_indent() { indent_ = false;}
   static void enable_indent()  { indent_ = true;}
   static bool indent_;
};

class DisableIndentor {
public:
   DisableIndentor() { Indentor::disable_indent(); }
   ~DisableIndentor() { Indentor::enable_indent();}
};

}
#endif
