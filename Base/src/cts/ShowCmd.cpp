/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #20 $ 
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
#include <iostream>
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "Str.hpp"
#include "CtsApi.hpp"
#include "AbstractClientEnv.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShowCmd::equals(ClientToServerCmd* rhs) const
{
	return (dynamic_cast<ShowCmd*>(rhs)) ?  UserCmd::equals(rhs) : false;
}

std::ostream& ShowCmd::print(std::ostream& os) const
{
	return user_cmd(os,"show");
}

STC_Cmd_ptr ShowCmd::doHandleRequest(AbstractServer* as) const
{
	/// Should never get called since, show command is only called on client side.
	return PreAllocatedReply::ok_cmd();
}

const char* ShowCmd::arg()  { return "show";}
const char* ShowCmd::desc() {
   return   "Used to print state of the definition returned from the server to standard output.\n"
	         "This command can *only* be used in a group command, and will only work if it is\n"
	         "preceded with a get command. See examples below.\n"
	         "   arg1 = [ defs | state | migrate ] \n"
	         "The output of show has several options: i.e\n"
	         "  o no arguments: With no arguments, print the definition structure to standard output\n"
            "    Extern's are automatically added, allowing the output to be reloaded into the server\n"
	         "    i.e --group=\"get ; show\"\n"
	         "  o state:\n"
	         "    This will output definition structure along with all the state information.\n"
            "    This will include the trigger expressions, abstract syntax tree as comments.\n"
            "    Excludes the edit history\n"
            "  o migrate:\n"
            "    This will output definition structure along with all the state information.\n"
            "    The node state is shown in the comments.\n"
            "    This format allows the definition to be migrated to future version of ecflow.\n"
            "    The output includes edit history but excludes externs.\n"
            "    When the definition is reloaded *NO* checking is done.\n"
            "\n"
            "The following shows a summary of the features associated with each choice\n"
            "                        DEFS          STATE      MIGRATE\n"
            "Auto generate externs   Yes           Yes        No\n"
            "Checking on reload      Yes           Yes        No\n"
            "Edit History            No            No         Yes\n"
            "trigger AST             No            Yes        No\n"
            "\n"
            "Usage:\n"
            "    --group=\"get ; show\"\n"
            "    --group=\"get ; show defs\"    # same as the previous example\n"
            "    --group=\"get ; show state\"   # Show all state for the node tree\n"
            "    --group=\"get ; show migrate\" # Shows state and allows migration\n"
	         "    --group=\"get=/s1; show\"      # show state for the node only\n"
	         "    --group=\"get=/s1; show state\""
	;
}

void ShowCmd::addOption(boost::program_options::options_description& desc) const {
 	desc.add_options()( ShowCmd::arg(), po::value< string >()->implicit_value(string()), ShowCmd::desc() );
}
void ShowCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* ac ) const
{
   std::string show_state = vm[ ShowCmd::arg() ].as< std::string > ();

   if (ac->debug()) cout << "  ShowCmd::create api = '" << show_state << "'.\n";

	PrintStyle::Type_t style = PrintStyle::DEFS;
	if (!show_state.empty()) {
	   if (show_state == "state")  style = PrintStyle::STATE;
	   else if (show_state == "migrate" ) style = PrintStyle::MIGRATE;
	   else if (show_state == "defs" ) style = PrintStyle::DEFS;
	   else throw std::runtime_error("ShowCmd::create invalid show option expected one of [ defs | state | migrate ] but found " + show_state);
	}
 	cmd = Cmd_ptr( new ShowCmd( style ) );
}

std::ostream& operator<<(std::ostream& os, const ShowCmd& c) { return c.print(os); }
