//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #19 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <assert.h>
#include <sstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/exception.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include "Alias.hpp"
#include "Defs.hpp"
#include "Ecf.hpp"
#include "Log.hpp"
#include "Stl.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "Indentor.hpp"
#include "PrintStyle.hpp"

namespace fs = boost::filesystem;
using namespace ecf;
using namespace std;
using namespace boost;

//==================================================================================
Alias::Alias( const std::string& name )
: Submittable(name)
{
   set_state_only(NState::QUEUED);
}

Alias::Alias( const Alias& rhs)
: Submittable(rhs)
{
}

Alias::Alias()
{
   set_state_only(NState::QUEUED);
}

Alias::~Alias()
{
   if (!Ecf::server()) {
      notify_delete();
   }
}

alias_ptr Alias::create(const std::string& name)
{
   return boost::make_shared<Alias>( name );
}

bool Alias::operator==(const Alias& rhs) const
{
   return Submittable::operator==(rhs);
}

std::ostream& Alias::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << "alias " << name();
   if (!PrintStyle::defsStyle()) {
      std::string st = write_state();
      if (!st.empty()) os << " #" << st;
   }
   os << "\n";

   Node::print(os);

   // Generated variable are not persisted since they are created on demand
   // There *NO* point in printing them they will always be empty
   return os;
}
std::ostream& operator<<(std::ostream& os, const Alias& d)  { return d.print(os); }

void Alias::begin()
{
   Submittable::begin();
}

void Alias::requeue(bool resetRepeats, int clear_suspended_in_child_nodes,bool reset_next_time_slot)
{
   Submittable::requeue(resetRepeats,clear_suspended_in_child_nodes,reset_next_time_slot);
}

const std::string& Alias::debugType() const { return ecf::Str::ALIAS();}

node_ptr Alias::removeChild(Node*)
{
   LOG_ASSERT(false,"");
   return node_ptr();
}

bool Alias::addChild( node_ptr , size_t )
{
   LOG_ASSERT(false,"");
   return false;
}

size_t Alias::child_position(const Node*) const
{
   return std::numeric_limits<std::size_t>::max();
}

bool Alias::isAddChildOk( Node* alias, std::string& errorMsg) const
{
   errorMsg += "Can not add children to a Alias";
   return false;
}

void Alias::handleStateChange()
{
   /// Increment/decrement limits based on the current state
   update_limits();

   // Aliases are stand alone, they do no requeue or bubble up/down state changes
   // i.e no requeue since they have no time dependencies, or repeat
}

const std::string& Alias::script_extension() const
{
   return File::USR_EXTN();
}

void Alias::collateChanges(DefsDelta& changes) const
{
   /// All changes to Alias should be on ONE compound_memento_ptr
   compound_memento_ptr comp;
   Submittable::incremental_changes(changes, comp);
}

void Alias::get_all_nodes(std::vector<node_ptr>& nodes) const
{
   nodes.push_back(non_const_this());
}

// Functions unique to aliases
void Alias::add_alias_variable(const std::string& name, const std::string& value)
{
   if (name.empty()) {
      throw std::runtime_error("Alias::add_alias_variable: Variable with empty name");
   }

   // The bool argument to variable, allows addition of Variable without name checking
   addVariable( Variable(name,value,false));
}

node_ptr Alias::find_node_up_the_tree(const std::string& name) const
{
   Node* the_parent = parent();
   if (the_parent) return the_parent->find_node_up_the_tree(name);
   return node_ptr();
}

