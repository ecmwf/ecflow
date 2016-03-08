/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $ 
//
// Copyright 2009-2012 ECMWF. 
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
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Log.hpp"
#include "Ecf.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

std::ostream& CSyncCmd::print(std::ostream& os) const
{
   /// Note: Be careful how the *debug* output is interpreted, since the:
   /// client_handle_  > 0  state/modify numbers will be for a set of registered suites,
   /// client_handle_ == 0  state/modify numbers is global i.e. for all suites.
   std::stringstream ss;
   if (api_ == CSyncCmd::NEWS) {
      ss << CtsApi::to_string(CtsApi::news(client_handle_,client_state_change_no_,client_modify_change_no_));
      return user_cmd(os,ss.str());
   }

	if (api_ == CSyncCmd::SYNC) {
	   ss << CtsApi::to_string(CtsApi::sync(client_handle_,client_state_change_no_,client_modify_change_no_));
	   return user_cmd(os,ss.str());
 	}

	ss <<  CtsApi::sync_full(client_handle_);
	return user_cmd(os,ss.str());
}

bool CSyncCmd::equals(ClientToServerCmd* rhs) const
{
	CSyncCmd* the_rhs = dynamic_cast< CSyncCmd* > ( rhs );
	if ( !the_rhs ) return false;
	if (api_ != the_rhs->api()) return false;
	if (client_handle_ != the_rhs->client_handle()) return false;
	if (client_state_change_no_ != the_rhs->client_state_change_no()) return false;
	if (client_modify_change_no_ != the_rhs->client_modify_change_no()) return false;
	return UserCmd::equals(rhs);
}

const char* CSyncCmd::theArg() const
{
   if (api_ == CSyncCmd::NEWS) return CtsApi::newsArg();
   if (api_ == CSyncCmd::SYNC) return CtsApi::syncArg();
	return CtsApi::sync_full_arg();
}

int CSyncCmd::timeout() const
{
   if (api_ == CSyncCmd::SYNC || api_ == CSyncCmd::SYNC_FULL) {
      return time_out_for_load_sync_and_get();
   }
   return 20; // CSyncCmd::NEWS
}

void CSyncCmd::do_log(AbstractServer* as) const
{
   if (api_ == CSyncCmd::NEWS)  {

      /// Log without adding a new line, to the log file
      /// The SNewsCmd will append additional debug and then add new line
      std::stringstream ss;
      print(ss);                                   // Populate the stream with command details:
      bool ok = log_no_newline(Log::MSG,ss.str()); // log command without adding newline
      if (!ok && as->defs()) {
         // problems writing to log file, warn user ECFLOW-536
         as->defs()->flag().set(ecf::Flag::LATE);
      }
      return;
   }

   ClientToServerCmd::do_log(as);
}

STC_Cmd_ptr CSyncCmd::doHandleRequest(AbstractServer* as) const
{
	// If no defs not loaded, SSyncCmd and SNewsCmd do nothing. This is a valid state, hence don't error for this request
	if (api_ == CSyncCmd::NEWS)  {
	   as->update_stats().news_++;
	   return PreAllocatedReply::news_cmd(client_handle_, client_state_change_no_, client_modify_change_no_,as);
 	}
	if (api_ == CSyncCmd::SYNC) {
      as->update_stats().sync_++;
		return PreAllocatedReply::sync_cmd(client_handle_, client_state_change_no_, client_modify_change_no_,as);
	}
   as->update_stats().sync_++;
   return PreAllocatedReply::sync_full_cmd(client_handle_,as);
}

void CSyncCmd::addOption(boost::program_options::options_description& desc) const
{
   if (api_ == CSyncCmd::NEWS)  {
      desc.add_options()(CtsApi::newsArg(),po::value< vector<unsigned int> >()->multitoken(),
               "Returns true if state of server definition changed.\n"
               "*Important* for use with c++/python interface only.\n"
               "Requires Given a client handle, change and modify number determine if server changed since last call\n"
               "This relies on user calling sync after news to update the locally stored modify and change numbers.\n"
               "These numbers are then used in the next call to news."
      );
      return;
   }

   if (api_ == CSyncCmd::SYNC)  {
      desc.add_options()(CtsApi::syncArg(),po::value< vector<unsigned int> >()->multitoken(),
               "Incrementally synchronise the local definition with the one in the server.\n"
               "*Important* for use with c++/python interface only.\n"
               "Preference should be given to this method as only the changes are returned.\n"
               "This reduces the network bandwidth required to keep in sync with the server\n"
               "Requires a client handle, change and modify number, to get the incremental changes from server.\n"
               "The change in server state is then and merged with the client definition."
      );
      return;
   }

   desc.add_options()(CtsApi::sync_full_arg(),po::value<  unsigned int >(),
            "Returns the full definition from the server.\n"
            "*Important* for use with c++/python interface only.\n"
            "Requires a client_handle. The returned definition is stored on the client."
    );
}

void CSyncCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv*  ac ) const
{
	if (ac->debug()) cout << "CSyncCmd::create api = '" << api_ << "'.\n";

	if (api_ == CSyncCmd::NEWS || api_ == CSyncCmd::SYNC){
	   vector<unsigned int> args = vm[ theArg() ].as< vector<unsigned int> >();
	   if (args.size() != 3) throw std::runtime_error("CSyncCmd::create(SYNC/NEWS) expects 3 integer arguments, Client handle, state change number, and modify change number");
	   unsigned int client_handle    = args[0];
	   unsigned int state_change_no  = args[1];
	   unsigned int modify_change_no = args[2];
	   cmd = Cmd_ptr(new CSyncCmd( api_,client_handle,state_change_no,modify_change_no));
	   return;
	}

   unsigned int client_handle = vm[ theArg() ].as< unsigned int >();
   cmd = Cmd_ptr(new CSyncCmd(client_handle)); // FULL_SYNC
}

std::ostream& operator<<(std::ostream& os, const CSyncCmd& c) { return c.print(os); }
