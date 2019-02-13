/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #31 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/lexical_cast.hpp>

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

/// *****************************************************************************
/// Note: The Client Handle commands, change the server,
///       However the changes cant really be considered as incremental
///       Hence for all handle command, we will do a FULL SYNC
///       Relying on the modify_change_no is to CRUDE, as it affects all handles
///       Instead we will use a simple flag to indicate that a FULL sync is required
/// *****************************************************************************

std::ostream& ClientHandleCmd::print(std::ostream& os) const
{
	switch (api_) {
		case ClientHandleCmd::REGISTER: return user_cmd(os,CtsApi::to_string(CtsApi::ch_register(client_handle_,auto_add_new_suites_,suites_))); break;
      case ClientHandleCmd::DROP:     return user_cmd(os,CtsApi::ch_drop(client_handle_)); break;
      case ClientHandleCmd::DROP_USER:{
         if (drop_user_.empty()) return user_cmd(os,CtsApi::ch_drop_user(user()));
         return user_cmd(os,CtsApi::ch_drop_user(drop_user_));
      }
		case ClientHandleCmd::ADD:      return user_cmd(os,CtsApi::to_string(CtsApi::ch_add(client_handle_,suites_))); break;
		case ClientHandleCmd::REMOVE:   return user_cmd(os,CtsApi::to_string(CtsApi::ch_remove(client_handle_,suites_))); break;
      case ClientHandleCmd::AUTO_ADD: return user_cmd(os,CtsApi::to_string(CtsApi::ch_auto_add(client_handle_,auto_add_new_suites_))); break;
      case ClientHandleCmd::SUITES:     return user_cmd(os,CtsApi::ch_suites()); break;
		default: assert(false); break;
 	}
  	return os;
}

std::ostream& ClientHandleCmd::print_only(std::ostream& os) const
{
   switch (api_) {
      case ClientHandleCmd::REGISTER: os << CtsApi::to_string(CtsApi::ch_register(client_handle_,auto_add_new_suites_,suites_)); break;
      case ClientHandleCmd::DROP:     os << CtsApi::ch_drop(client_handle_); break;
      case ClientHandleCmd::DROP_USER:{
         if (drop_user_.empty()) os << CtsApi::ch_drop_user(user());
         else                    os << CtsApi::ch_drop_user(drop_user_);
         break;
      }
      case ClientHandleCmd::ADD:      os << CtsApi::to_string(CtsApi::ch_add(client_handle_,suites_)); break;
      case ClientHandleCmd::REMOVE:   os << CtsApi::to_string(CtsApi::ch_remove(client_handle_,suites_)); break;
      case ClientHandleCmd::AUTO_ADD: os << CtsApi::to_string(CtsApi::ch_auto_add(client_handle_,auto_add_new_suites_)); break;
      case ClientHandleCmd::SUITES:   os << CtsApi::ch_suites(); break;
      default: assert(false); break;
   }
   return os;
}


bool ClientHandleCmd::equals(ClientToServerCmd* rhs) const
{
	auto* the_rhs = dynamic_cast< ClientHandleCmd* > ( rhs );
	if ( !the_rhs ) return false;
   if (api_ != the_rhs->api()) return false;
   if (drop_user_ != the_rhs->drop_user()) return false;
 	return UserCmd::equals(rhs);
}

const char* ClientHandleCmd::theArg() const
{
	switch (api_) {
		case ClientHandleCmd::REGISTER:  return CtsApi::ch_register_arg(); break;
      case ClientHandleCmd::DROP:      return CtsApi::ch_drop_arg(); break;
      case ClientHandleCmd::DROP_USER: return CtsApi::ch_drop_user_arg(); break;
		case ClientHandleCmd::ADD:       return CtsApi::ch_add_arg(); break;
		case ClientHandleCmd::REMOVE:    return CtsApi::ch_remove_arg(); break;
      case ClientHandleCmd::AUTO_ADD:  return CtsApi::ch_auto_add_arg(); break;
      case ClientHandleCmd::SUITES:    return CtsApi::ch_suites_arg(); break;
 	}
	assert(false);
	return nullptr;
}

bool ClientHandleCmd::cmd_updates_defs() const
{
   switch (api_) {
      case ClientHandleCmd::REGISTER:  return true; break;
      case ClientHandleCmd::DROP:      return true; break; // can be expensive for large defs
      case ClientHandleCmd::DROP_USER: return true; break; // can be expensive for large defs
      case ClientHandleCmd::ADD:       return true; break;
      case ClientHandleCmd::REMOVE:    return true; break;
      case ClientHandleCmd::AUTO_ADD:  return false; break;
      case ClientHandleCmd::SUITES:    return false; break;
   }
   assert(false);
   return false;
}

STC_Cmd_ptr ClientHandleCmd::doHandleRequest(AbstractServer* as) const
{
   as->update_stats().ch_cmd_++;

	switch (api_) {
		case ClientHandleCmd::REGISTER:  {

		   // If existing handle is non zero drop it first
		   if (client_handle_ != 0) {
		      as->defs()->client_suite_mgr().remove_client_suite(client_handle_); // will throw if handle not found
		   }

 			unsigned int client_handle = as->defs()->client_suite_mgr().create_client_suite(auto_add_new_suites_,suites_,user());
//#ifdef DEBUG
// 	   LOG(Log::DBG,as->defs()->client_suite_mgr().dump_max_change_no());
//#endif

 			// If this command is part of a group command, let the following sync command, know about the new handle
 			// So it return the defs with the right set of suites.
 			if (group_cmd_) group_cmd_->set_client_handle(client_handle);

			// return the handle to the client
		 	return PreAllocatedReply::client_handle_cmd(client_handle) ;
		}

		case ClientHandleCmd::DROP: {
		   as->defs()->client_suite_mgr().remove_client_suite(client_handle_); // will throw if handle not found

         // If this command is part of a group command, let the following sync command, know about the new handle
         // So it return the defs with the right set of suites.
		   // If used with following sync, will return *FULL* defs. Can be expensive
         if (group_cmd_) group_cmd_->set_client_handle(0);

		   // return the 0 handle to the client. The client stores the handle locally. Reset to zero.
		   return PreAllocatedReply::client_handle_cmd(0) ;
		}

		case ClientHandleCmd::DROP_USER: {
		   // will throw if no users handles dropped
		   if (drop_user_.empty()) as->defs()->client_suite_mgr().remove_client_suites(user());
		   else                    as->defs()->client_suite_mgr().remove_client_suites(drop_user_);

		   if (drop_user_.empty() || drop_user_ == user()) {

	         // If this command is part of a group command, let the following sync command, know about the new handle
	         // So it return the defs with the right set of suites.
	         // If used with following sync, will return *FULL* defs.
	         if (group_cmd_) group_cmd_->set_client_handle(0);

		      // return the 0 handle to the client. The client stores the handle locally. Reset to zero.
		      return PreAllocatedReply::client_handle_cmd(0) ;
		   }
		   break;
		}

		case ClientHandleCmd::ADD:  {
		   as->defs()->client_suite_mgr().add_suites(client_handle_,suites_);  // will throw if handle not found
 			break;
		}

		case ClientHandleCmd::REMOVE:  {
		   as->defs()->client_suite_mgr().remove_suites(client_handle_,suites_);  // will throw if handle not found
  			break;
		}

		case ClientHandleCmd::AUTO_ADD: {
		   as->defs()->client_suite_mgr().auto_add_new_suites(client_handle_,auto_add_new_suites_);  // will throw if handle not found
			break;
		}

      case ClientHandleCmd::SUITES: {
         return PreAllocatedReply::client_handle_suites_cmd(as) ;
      }

		default: assert(false); break;
 	}
	return PreAllocatedReply::ok_cmd();
}

void ClientHandleCmd::addOption(boost::program_options::options_description& desc) const
{
	switch (api_) {
		case ClientHandleCmd::REGISTER:{
			desc.add_options()(CtsApi::ch_register_arg(), po::value< vector<string> >()->multitoken(),
			         "Register interest in a set of suites.\n"
			         "If a definition has lots of suites, but the client. is only interested in a small subset,\n"
			         "Then using this command can reduce network bandwidth and synchronisation will be quicker.\n"
			         "This command will create a client handle, which must be used for any other changes.\n"
			         "The newly created handle can be shown with the --ch_suites command\n"
			         "Deleted suites will stay registered, and must be explicitly removed/dropped.\n"
			         "Note: Suites can be registered before they are loaded into the server\n"
			         "This command affects news() and sync() commands\n"
			         "   arg1 = true | false           # true means add new suites to my list, when they are created\n"
			         "   arg2 = names                  # should be a list of suite names, names not in the definition are ignored\n"
			         "Usage:\n"
			         "   --ch_register=true s1 s2 s3   # register interest in suites s1,s2,s3 and any new suites\n"
			         "   --ch_register=false s1 s2 s3  # register interest in suites s1,s2,s3 only\n"
			         "   --ch_register=false           # register handle, suites will be added later on\n"
                  "   --ch_register=1 true s1 s2 s3 # drop handle 1 then register interest in suites s1,s2,s3 and any new suites\n"
			         "                                 # The client handle as the first argument is typically used by GUI/python"
			         "                                 # When the client handle is no zero, then it is dropped first\n"
			         "To list all suites and handles use --ch_suites"
 			);
			break;
		}

		case ClientHandleCmd::DROP:{
			desc.add_options()(CtsApi::ch_drop_arg(), po::value< int >(),
			         "Drop/de-register the client handle.\n"
			         "Un-used handle should be dropped otherwise they will stay, in the server.\n"
			         "   arg1 = handle(integer)  # The handle must be an integer that is > 0\n"
			         "Usage:\n"
			         "   --ch_drop=10            # drop the client handle 10\n"
			         "An error is returned if the handle had not previously been registered\n"
			         "The handle stored on the local client is set to zero\n"
			         "To list all suites and handles use --ch_suites"
 			);
			break;
		}

      case ClientHandleCmd::DROP_USER:{
         desc.add_options()(CtsApi::ch_drop_user_arg(), po::value<std::string >()->implicit_value( string("")),
                  "Drop/de-register all handles associated with the given user.\n"
                  "If no user provided will drop for current user. Client must ensure un-used handle are dropped\n"
                  "otherwise they will stay, in the server.\n"
                  "   arg1 = user           # The user to be drooped, if left empty drop current user \n"
                  "Usage:\n"
                  "   --ch_drop_user=ma0    # drop all handles associated with user ma0\n"
                  "   --ch_drop_user        # drop all handles associated with current user\n"
                  "An error is returned if no registered handles\n"
                  "To list all suites and handles use --ch_suites"
         );
         break;
      }

		case ClientHandleCmd::ADD:{
			desc.add_options()( CtsApi::ch_add_arg(), po::value< vector<string> >()->multitoken(),
					"Add a set of suites, to an existing handle.\n"
					"   arg1 = handle(integer)  # The handle must be an integer that is > 0\n"
 					"   arg2 = names            # should be a list of suite names, names not in the definition are ignored\n"
					"Usage:\n"
					"   --ch_add=10 s2 s3 s4    # add suites s2 s3,s4 to  handle 10\n"
					"An error is returned if the handle had not previously been registered\n"
					"The handle is created with --ch_register command\n"
					"To list all suites and handles use --ch_suites"
			);
			break;
		}

		case ClientHandleCmd::REMOVE:{
			desc.add_options()( CtsApi::ch_remove_arg(), po::value< vector<string> >()->multitoken(),
					"Remove a set of suites, from an existing handle.\n"
					"   arg1 = handle(integer)   # The handle must be an integer that is > 0\n"
					"   arg2 = names             # should be a list of suite names, names not in the definition are ignored\n"
					"Usage:\n"
					"   --ch_rem=10 s2 s3 s4     # remove suites s2 s3,s4 from handle 10\n"
					"The handle is created with --ch_register command\n"
					"To list all suites and handles use --ch_suites"
			);
 			break;
		}

		case ClientHandleCmd::AUTO_ADD: {
			desc.add_options()( CtsApi::ch_auto_add_arg(), po::value< vector<string> >()->multitoken(),
					"Change an existing handle so that new suites can be added automatically.\n"
					"   arg1 = handle(integer)  # The handle must be an integer that is > 0\n"
					"   arg2 = true | false     # true means add new suites to my list, when they are created\n"
					"Usage:\n"
					" --ch_auto_add=10 true     # modify handle 10 so that new suites, get added automatically to it\n"
					" --ch_auto_add=10 false    # modify handle 10 so that no new suites are added\n"
					"The handle is created with --ch_register command\n"
					"To list all suites and handles use --ch_suites"
			);
			break;
		}

      case ClientHandleCmd::SUITES:{
         desc.add_options()(CtsApi::ch_suites_arg(),
                  "Shows all the client handles, and the suites they reference"
          );
         break;
      }
		default: assert(false); break;
 	}
}

void ClientHandleCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv*  ac ) const
{
	if (ac->debug()) cout << "  ClientHandleCmd::create api = '" << api_ << "'.\n";

	switch (api_) {

		case ClientHandleCmd::REGISTER:  {
			vector<string> args = vm[  theArg() ].as< vector<string> >();
			// args can be empty, otherwise first arg must be integer or bool true or false, subsequent args represent suite names
			int client_handle = 0;
			bool auto_add_new_suites = false;
			std::vector<std::string> suite_names; suite_names.reserve( args.size() );
			int suite_names_index = 1;
			if (!args.empty()) {
			   try {
			      client_handle = boost::lexical_cast<int>(args[0]);
			      if (args.size() > 1) {
			         if (args[1] == "true") auto_add_new_suites = true;
			         else if (args[1] == "false") auto_add_new_suites = false;
			         suite_names_index = 2;
			      }
			   }
			   catch (...) {
			      if (args[0] == "true") auto_add_new_suites = true;
			      else if (args[0] == "false") auto_add_new_suites = false;
			      else throw std::runtime_error("ClientHandleCmd::create: First argument should be integer | true | false. See help");
			   }
				for(size_t i = suite_names_index; i < args.size(); i++) { suite_names.push_back( args[i] ); }
			}
		 	cmd = std::make_shared<ClientHandleCmd>(client_handle, suite_names, auto_add_new_suites );
			break;
		}

 		case ClientHandleCmd::DROP:  {
			int client_handle = vm[  theArg() ].as< int >();
			if ( 0 == client_handle) throw std::runtime_error("ClientHandleCmd::create: handles must have a value > 0");
		 	cmd = std::make_shared<ClientHandleCmd>( client_handle );
 			break;
 		}

      case ClientHandleCmd::DROP_USER:  {
         std::string the_user_to_drop = vm[  theArg() ].as< std::string >();
         cmd = std::make_shared<ClientHandleCmd>( the_user_to_drop );
         break;
      }

		case ClientHandleCmd::ADD:   {
			vector<string> args = vm[  theArg() ].as< vector<string> >();
 			if (args.size() < 2) throw std::runtime_error("To few arguments. First arg should be a integer handle, then a list of suite names. See help");
 			int client_handle = 0;
 			try { client_handle = boost::lexical_cast<int>( args[0]); }
 			catch (std::exception& ) { throw std::runtime_error("The first argument must be an integer. See help"); }
			if (0 == client_handle) throw std::runtime_error("ClientHandleCmd::create: handles must have a value > 0");
 			std::vector<std::string> suite_names; suite_names.reserve( args.size() );
			for(size_t i = 1; i < args.size(); i++) { suite_names.push_back( args[i] ); }
		 	cmd = std::make_shared<ClientHandleCmd>(client_handle, suite_names, ClientHandleCmd::ADD );
			break;
		}

		case ClientHandleCmd::REMOVE:  {
			vector<string> args = vm[  theArg() ].as< vector<string> >();
 			if (args.size() < 2) throw std::runtime_error("To few arguments. First arg should be a integer handle, then a list of suite names. See help");
 			int client_handle = 0;
 			try { client_handle = boost::lexical_cast<int>( args[0]); }
 			catch (std::exception& ) { throw std::runtime_error("ClientHandleCmd::create: The first argument must be an integer. See help"); }
			if ( 0 == client_handle ) throw std::runtime_error("ClientHandleCmd::create: handles must have a value > 0");
 			std::vector<std::string> suite_names; suite_names.reserve( args.size() );
			for(size_t i = 1; i < args.size(); i++) { suite_names.push_back( args[i] ); }
		 	cmd = std::make_shared<ClientHandleCmd>(client_handle, suite_names, ClientHandleCmd::REMOVE );
			break;
		}

 		case ClientHandleCmd::AUTO_ADD:  {
			vector<string> args = vm[  theArg() ].as< vector<string> >();
 			if (args.size() != 2) throw std::runtime_error("Two argument expected. First arg should be a integer handle, second should be true or false. See help");
 			int client_handle = 0;
 			try { client_handle = boost::lexical_cast<int>( args[0]); }
 			catch (std::exception& ) { throw std::runtime_error("ClientHandleCmd::create: The first argument must be an integer. See help"); }
			if ( 0 == client_handle ) throw std::runtime_error("ClientHandleCmd::create: handles must have a value > 0");
			bool auto_add_new_suites =  false;
			if (args[1] == "true") auto_add_new_suites = true;
			else if (args[1] == "false") auto_add_new_suites = false;
			else throw std::runtime_error("ClientHandleCmd::create: First argument should be true | false. See help");
		 	cmd = std::make_shared<ClientHandleCmd>(client_handle, auto_add_new_suites );
 			break;
 		}

      case ClientHandleCmd::SUITES:  {
         cmd = std::make_shared<ClientHandleCmd>( ClientHandleCmd::SUITES );
         break;
      }

		default: assert(false); break;
 	}
}

std::ostream& operator<<(std::ostream& os, const ClientHandleCmd& c) { return c.print(os); }
