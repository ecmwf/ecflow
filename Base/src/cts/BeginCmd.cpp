/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "CtsApi.hpp"
#include "AbstractClientEnv.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;


BeginCmd::BeginCmd(const std::string& suiteName, bool force)
: suiteName_(suiteName), force_(force)
{
   // The begin command actually requires the suite name without the lead '/'
   // i.e if we provide /suite --> change to suite
   if (!suiteName_.empty() && suiteName_[0] == '/') {
      suiteName_.erase(0,1); // delete first character
   }
}

std::ostream& BeginCmd::print(std::ostream& os) const
{
   return user_cmd(os,CtsApi::begin(suiteName_,force_));
}

std::ostream& BeginCmd::print_only(std::ostream& os) const
{
   os << CtsApi::begin(suiteName_,force_); return os;
}

bool BeginCmd::equals(ClientToServerCmd* rhs) const
{
	auto* the_rhs = dynamic_cast< BeginCmd* > ( rhs );
	if ( !the_rhs ) return false;
	if (suiteName_ != the_rhs->suiteName()) return false;
	if (force_ != the_rhs->force()) return false;
	return UserCmd::equals(rhs);
}

STC_Cmd_ptr BeginCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().begin_cmd_++;
	defs_ptr defs = as->defs();

   std::vector<Submittable*> tasks;

	// If no suite name begin all suites, else begin the the specific suite
	if ( suiteName_.empty()) {

		const std::vector<suite_ptr>& suiteVec = defs->suiteVec();
		size_t theSuiteVecSize = suiteVec.size();
		if (!force_) {
			for(size_t s = 0; s < theSuiteVecSize; s++) {
				/// check_suite_can_begin will throw if suite can't begin
				defs->check_suite_can_begin(suiteVec[s]);
			}
		}
		else {
		   defs->get_all_active_submittables(tasks);
		   as->zombie_ctrl().add_user_zombies(tasks,CtsApi::beginArg());

		   defs->reset_begin();  // Force should *only* be used for test
		}

		defs->beginAll();
	}
	else {

		suite_ptr suite = defs->findSuite(suiteName_);
		if (!suite.get()) {
		    std::stringstream ss;
		    ss << "BeginCmd::doHandleRequest:  Begin failed as suite '"  << suiteName_ << "' is not loaded.\n";
		    throw std::runtime_error( ss.str() );
	 	}

		/// check_suite_can_begin will throw if suite can't begin
 		if (!force_) defs->check_suite_can_begin(suite);
		else  {

		   suite->get_all_active_submittables(tasks);
		   as->zombie_ctrl().add_user_zombies(tasks,CtsApi::beginArg());

		   suite->reset_begin();  // Force should *only* be used for test
		}

 		defs->beginSuite(suite);
	}

	// The begin will clear the zombie flag: Hence reset it here.
	for(auto task : tasks) task->flag().set(ecf::Flag::ZOMBIE);


	// After begin do the first Job submission. This will kick of those
	// jobs that have no dependencies, or relative time of +00:00
	return doJobSubmission(as);
}

const char* BeginCmd::arg() { return CtsApi::beginArg();}
const char* BeginCmd::desc() {
	         /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   return
            "Begin playing the definition in the server.\n"
            "Expects zero or a single quoted string.\n"
            "  arg1 = suite-name | Nothing | force\n"
            "         play the chosen suite, if no arg specified, play all suites, in the definition\n"
            "         force means reset the begin status on the suites and bypass checks.\n"
            "         This is only required if suite-name is provide as the first argument\n"
            "         Using force can cause the creation of zombies\n"
            "Usage:\n"
            "--begin                     # will begin all suites\n"
            "--begin=\"--force\"         # reset and then begin all suites, bypassing any checks. Note: string must be quoted\n"
            "--begin=\"mySuite\"         # begin playing suite of name 'mySuite'\n"
            "--begin=\"mySuite --force\" # reset and begin playing suite 'mySuite', bypass check"
	;
}

void BeginCmd::addOption(boost::program_options::options_description& desc) const
{
	// allow options like
	// client --begin=suitename       // begin <suitename>
	// client --begin                 // means begin all suites
	desc.add_options()( BeginCmd::arg(),po::value< string >()->implicit_value( string("") ), BeginCmd::desc() );
}
void BeginCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* ace ) const
{
	std::string beginArg = vm[ arg() ].as< std::string > ();

	if (ace->debug()) {
		cout << "  BeginCmd::create arg = " << beginArg << "\n";
 	}

	std::string suiteName;
	bool force = false;

	if (!beginArg.empty()) {
		std::vector< std::string > lineTokens;
		Str::split(beginArg,lineTokens);
		if (lineTokens.size() == 1) {
			if (lineTokens[0] == "--force") force = true;
			else suiteName = lineTokens[0];
		}
		else if (lineTokens.size() == 2) {
			suiteName = lineTokens[0];
			if (lineTokens[1] != "--force") {
				std::stringstream ss;
				ss << "BeginCmd: Expected second argument to be '--force' but found " << lineTokens[1] << "\n";
				throw std::runtime_error( ss.str() );
			}
			force = true;
		}
		else {
			std::stringstream ss;
			ss << "BeginCmd: Expect zero, one or 2 arguments, but found " << lineTokens.size() << " arguments\n" << BeginCmd::desc() << "\n";
			throw std::runtime_error( ss.str() );
		}
	}

   if (ace->debug()) {
      std::cout << "  BeginCmd::create suiteName = " << suiteName << "\n";
      std::cout << "  BeginCmd::create force = " << force << "\n";
   }

	cmd = Cmd_ptr(new BeginCmd( suiteName, force ));
}

std::ostream& operator<<(std::ostream& os, const BeginCmd& c) { return c.print(os); }
