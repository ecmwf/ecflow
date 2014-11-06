//============================================================================
// Name        : NodeTree.cpp
// Author      : Avi
// Revision    : $Revision: #64 $ 
//
// Copyright 2009-2012 ECMWF. 
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
#include <boost/make_shared.hpp>

#include "Family.hpp"
#include "Log.hpp"
#include "PrintStyle.hpp"
#include "NodeTreeVisitor.hpp"
#include "ChangeMgrSingleton.hpp"
#include "Ecf.hpp"

#include "Stl.hpp"
#include "Str.hpp"
#include "Indentor.hpp"
#include "DefsDelta.hpp"
#include "JobProfiler.hpp"

using namespace ecf;
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////
// The false below is used as a dummy argument to call the Variable constructor that does not
// Check the variable names. i.e we know they are valid
Family::~Family()
{
   // Don't create the ChangeMgrSingleton during destruct sequence. (i.e in unit cases)
   // Since that will cause a memory leak
   if (!Ecf::server() && ChangeMgrSingleton::exists()) {
      ChangeMgrSingleton::instance()->notify_delete( this );
   }

   delete fam_gen_variables_;
}

family_ptr Family::create(const std::string& name)
{
	return boost::make_shared<Family>( name );
}

void Family::accept(ecf::NodeTreeVisitor& v)
{
	v.visitFamily(this);
	NodeContainer::accept(v);
}

void Family::acceptVisitTraversor(ecf::NodeTreeVisitor& v)
{
	v.visitFamily(this);
}

void Family::begin()
{
   NodeContainer::begin();
   update_generated_variables();
}

bool Family::resolveDependencies(JobsParam& jobsParam)
{
   JobProfiler profile_me(this,jobsParam,JobProfiler::family_threshold());
   if (profile_me.time_taken_for_job_generation_to_long()) return false;
   return NodeContainer::resolveDependencies(jobsParam);
}

void Family::requeue(bool resetRepeats, int clear_suspended_in_child_nodes, bool reset_next_time_slot)
{
   NodeContainer::requeue(resetRepeats,clear_suspended_in_child_nodes,reset_next_time_slot);
   update_generated_variables();
}

bool Family::operator==(const Family& rhs) const
{
	return NodeContainer::operator==(rhs);
}

std::ostream& Family::print(std::ostream& os) const
{
	// Generated variable are not persisted since they are created on demand
	// There *NO* point in printing them they will always be empty

	Indentor in;
	Indentor::indent(os) << "family " << name();
   if (!PrintStyle::defsStyle()) {
      std::string st = write_state();
      if (!st.empty()) os << " #" << st;
   }
   os << "\n";

	Node::print(os);
	NodeContainer::print(os);
	Indentor::indent(os) << "endfamily\n";
	return os;
}

std::string Family::write_state() const
{
   return NodeContainer::write_state();
}
void Family::read_state(const std::string& line,const std::vector<std::string>& lineTokens)
{
   NodeContainer::read_state(line,lineTokens);
}

const std::string& Family::debugType() const { return ecf::Str::FAMILY();}

std::ostream& operator<<(std::ostream& os, const Family& d) { return d.print(os); }

void Family::collateChanges(DefsDelta& changes) const
{
   /// All changes to family should be on ONE compound_memento_ptr
	compound_memento_ptr compound;
	NodeContainer::incremental_changes(changes, compound);

	// Traversal
	NodeContainer::collateChanges(changes);
}

// generated variables --------------------------------------------------------------------------

void Family::update_generated_variables() const
{
   if (!fam_gen_variables_) fam_gen_variables_ = new FamGenVariables(this);
   fam_gen_variables_->update_generated_variables();
   update_repeat_genvar();
}

const Variable& Family::findGenVariable(const std::string& name) const
{
   // Generally it should be never the case that the values are empty
   // Since the user is assumed to have called begin(), which force
   // the generation of generated variables

   // AST can reference generated variables. Currently integer based values
   // The family names can be integers

   if (!fam_gen_variables_) update_generated_variables();
   const Variable& gen_var = fam_gen_variables_->findGenVariable(name);
   if (!gen_var.empty()) return gen_var;

   return NodeContainer::findGenVariable(name);
}

void Family::gen_variables(std::vector<Variable>& vec) const
{
   if (!fam_gen_variables_) update_generated_variables();

   vec.reserve(vec.size() + 3);
   fam_gen_variables_->gen_variables(vec);
   NodeContainer::gen_variables(vec);
}

// ================================================================

FamGenVariables::FamGenVariables(const Family* f)
 : family_(f),
   genvar_family_("FAMILY", "", false),
   genvar_family1_("FAMILY1", "", false)  {}

void FamGenVariables::update_generated_variables() const
{
   // This function is called during:
    //   o begin()
    //   o requeue()
    // Since family generated not persisted, allow for demand creation by client
    genvar_family1_.set_value(family_->name());

    // FAMILY is the full path excluding the suite, there is *NO* leading slash
    std::string path = family_->absNodePath();
    string::size_type secondSlash = path.find('/',1);
    path.erase(0,secondSlash+1);
    genvar_family_.set_value(path);
}

const Variable& FamGenVariables::findGenVariable(const std::string& name) const
{
   if (genvar_family_.name() == name)  return genvar_family_;
   if (genvar_family1_.name() == name)  return genvar_family1_;
   return Variable::EMPTY();
}

void FamGenVariables::gen_variables(std::vector<Variable>& vec) const
{
   vec.push_back(genvar_family_);
   vec.push_back(genvar_family1_);
}
