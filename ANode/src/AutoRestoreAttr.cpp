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
#include "NodeContainer.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "Str.hpp"
#include "Serialization.hpp"

using namespace std;

namespace ecf {

void AutoRestoreAttr::print(std::string& os) const
{
   Indentor in;
   Indentor::indent(os); write(os); os += "\n";
}

std::string AutoRestoreAttr::toString() const
{
   std::string ret;
   write(ret);
   return ret;
}

void AutoRestoreAttr::write(std::string& ret) const
{
   ret += "autorestore";
   for(const auto & i : nodes_to_restore_) { ret += " "; ret += i;}
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

void AutoRestoreAttr::do_autorestore()
{
   string warning_message;
   for(const auto & i : nodes_to_restore_) {

      warning_message.clear();
      node_ptr referenceNode = node_->findReferencedNode( i , warning_message);
      if (!referenceNode.get()) {
         /// Could not find the references node
         std::stringstream ss;
         ss << "AutoRestoreAttr::do_auto_restore: " << node_->debugType() << " references a path '" << i  << "' which can not be found\n";
         log(Log::ERR,ss.str());
         continue;
      }

      NodeContainer* nc = referenceNode->isNodeContainer();
      if (nc) {
         try { nc->restore();}
         catch( std::exception& e) {
            std::stringstream ss; ss << "AutoRestoreAttr::do_auto_restore: could not autorestore : because : " << e.what();
            log(Log::ERR,ss.str());
         }
      }
      else {
         std::stringstream ss;
         ss << "AutoRestoreAttr::do_auto_restore: " << node_->debugType() << " references a node '" << i  << "' which can not be restored. Only family and suite nodes can be restored";
         log(Log::ERR,ss.str());
      }
   }
}

void AutoRestoreAttr::check(std::string& errorMsg) const
{
   std::vector<NodeContainer*> vec;
   string warning_message;
   for(const auto & i : nodes_to_restore_) {

      warning_message.clear();
      node_ptr referenceNode = node_->findReferencedNode( i , warning_message);
      if (!referenceNode.get()) {
         /// Could not find the references node

         // OK a little bit of duplication, since findReferencedNode, will also look for externs
         // See if the Path:name is defined as an extern, in which case *DONT* error:
         // This is client side specific, since server does not have externs.
         if (node_->defs()->find_extern( i, Str::EMPTY())) {
            continue;
         }

         std::stringstream ss;
         ss << "Error: autorestore on node " << node_->debugType() << " references a path '" << i  << "' which can not be found\n";
         errorMsg += ss.str();
         continue;
      }

      // reference node found, make sure it not a task
      NodeContainer* nc = referenceNode->isNodeContainer();
      if (!nc) {
          std::stringstream ss;
          ss << "Error: autorestore on node " << node_->debugType() << " references a node '" << i  << "' which is a task. restore only works with suites or family nodes";
          errorMsg += ss.str();
      }

      // Check for duplicate references
      if (find(vec.begin(),vec.end(),nc) == vec.end()) vec.push_back(nc);
      else {
         std::stringstream ss;
         ss << "Error: autorestore on node " << node_->debugType() << ", duplicate references to node '" << i  << "'";
         errorMsg += ss.str();
      }
   }
}


template<class Archive>
void AutoRestoreAttr::serialize(Archive & ar, std::uint32_t const version )
{
   ar(CEREAL_NVP(nodes_to_restore_));
}
CEREAL_TEMPLATE_SPECIALIZE_V(AutoRestoreAttr);


}
