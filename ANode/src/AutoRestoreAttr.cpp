//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <sstream>

#include "AutoRestoreAttr.hpp"
#include "Indentor.hpp"
#include "Ecf.hpp"

using namespace std;

namespace ecf {

std::ostream& AutoRestoreAttr::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << "autorestore";
   for(size_t i = 0; i < nodes_to_restore_.size(); ++i) os << " " << nodes_to_restore_[i];
   os << "\n";
   return os;
}

std::string AutoRestoreAttr::toString() const
{
   std::stringstream ss;
   ss << "autorestore";
   for(size_t i = 0; i < nodes_to_restore_.size(); ++i) os << " " << nodes_to_restore_[i];
   return ss.str();
}


bool AutoRestoreAttr::operator==(const AutoRestoreAttr& rhs) const
{
   if (nodes_to_restore_ == rhs.nodes_to_restore_) return true;

#ifdef DEBUG
   if (Ecf::debug_equality()) {
      std::cout << "AutoRestoreAttr::operator== nodes_to_restore_ == rhs.nodes_to_restore_\n";
   }
#endif

   return false;
}

void AutoRestoreAttr::do_auto_restore()
{

}

bool AutoRestoreAttr::check(std::string& errorMsg) const
{
   return true;
}


}
