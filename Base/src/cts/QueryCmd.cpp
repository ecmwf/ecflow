/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #20 $
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
#include <iostream>
#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "Str.hpp"
#include "CtsApi.hpp"
#include "AbstractClientEnv.hpp"
#include "Extract.hpp"
#include "Expression.hpp"
#include "Node.hpp"
#include "ExprAstVisitor.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//////////////////////////////////////////////////////////////////////////////////////////////////

QueryCmd::~QueryCmd() = default;

std::ostream& QueryCmd::print(std::ostream& os) const
{
   // path_to_task_ is only used in logging, so we know which task initiated the query, can be empty when invoked via cmd line
   std::string the_query_cmd = CtsApi::to_string(CtsApi::query(query_type_,path_to_attribute_,attribute_));
   the_query_cmd += path_to_task_;
   return user_cmd(os,the_query_cmd );
}
std::ostream& QueryCmd::print_only(std::ostream& os) const
{
   std::string the_query_cmd = CtsApi::to_string(CtsApi::query(query_type_,path_to_attribute_,attribute_));
   the_query_cmd += path_to_task_;
   os << the_query_cmd;
   return os;
}

bool QueryCmd::equals(ClientToServerCmd* rhs) const
{
   auto* the_rhs = dynamic_cast<QueryCmd*>(rhs);
   if (!the_rhs) return false;
   if (query_type_        != the_rhs->query_type()) return false;
   if (path_to_attribute_ != the_rhs->path_to_attribute()) return false;
   if (attribute_         != the_rhs->attribute()) return false;
   if (path_to_task_      != the_rhs->path_to_task()) return false;
   return UserCmd::equals(rhs);
}

void QueryCmd::addOption(boost::program_options::options_description& desc) const
{
   desc.add_options()( QueryCmd::arg(), po::value< vector<string> >()->multitoken(), QueryCmd::desc() );
}

void  QueryCmd::create(   Cmd_ptr& cmd,
                          boost::program_options::variables_map& vm,
                          AbstractClientEnv* clientEnv ) const
{
   vector<string> args = vm[ arg() ].as< vector<string> >();

   if (clientEnv->debug()) {
      dumpVecArgs(QueryCmd::arg(),args);
      cout << "  QueryCmd::create " << QueryCmd::arg() << " task_path(" << clientEnv->task_path() << ")\n";
   }

   std::string query_type;
   std::string path_to_attribute;
   std::string attribute;

   if (args.size()) query_type = args[0];
   if ( query_type == "event" || query_type == "meter" || query_type == "variable") {
      // second argument must be <path>:event_or_meter_or_variable
      std::string path_and_name ;
      if (args.size() == 2) {
         path_and_name = args[1];
         if ( !Extract::pathAndName( path_and_name , path_to_attribute, attribute  ) ) {
            throw std::runtime_error(
                     "QueryCmd: second argument must be of the form <path>:event_or_meter_or_var_name for query "
                     + query_type + " : could not extract from:" + path_and_name );
         }
      }
      else {
         std::stringstream ss; ss << "QueryCmd: second argument must be of the form <path>:event_or_meter_or_var_name for query " << query_type;
         ss << " args size = " << args.size() << " expected 2 arguments";
         throw std::runtime_error(ss.str());
      }
      if (attribute.empty()) throw std::runtime_error( "QueryCmd: no attribute specified: query type: " + query_type + " path+attribute: " + path_and_name + "\n" + string(QueryCmd::desc()) );
   }
   else if (query_type == "trigger") {
      for(size_t i=1; i < args.size(); i++){
         if (i == 1) path_to_attribute = args[i];
         if (i == 2) {
            attribute = args[i];
            (void)Expression::parse(attribute,"QueryCmd:"); // will throw if expression does not parse
         }
      }
      if (attribute.empty()) throw std::runtime_error( "QueryCmd: no attribute specified: query type: trigger\n" + string(QueryCmd::desc()) );
   }
   else if (query_type == "state" || query_type == "dstate") {
      // for state and dstate attribute is empty
      if (args.size() >= 1) path_to_attribute = args[1];
      if (args.size() > 2) throw std::runtime_error( "QueryCmd: invalid (state | dstate) query : " + args[2]);
   }
   else throw std::runtime_error( "QueryCmd: first argument must be one of [ state | dstate | event | meter | variable | trigger ] but found:" + query_type);

   if (path_to_attribute.empty() || (!path_to_attribute.empty() &&  path_to_attribute[0] != '/')) {
      throw std::runtime_error( "QueryCmd: invalid path to attribute: " + path_to_attribute);
   }

   // path_to_task can be empty if invoked via the command line. ( used for logging, i.e identifying which task invoked this command)
   // However if invoked from the shell/python we expect the path_to_task(ECF_NAME) to have been set
   std::string path_to_task = clientEnv->task_path(); // can be empty, when cmd called from command line
   if (!path_to_task.empty() && path_to_task[0] != '/') {
      throw std::runtime_error( "QueryCmd: invalid path to task: " +  path_to_task);
   }

   cmd = std::make_shared<QueryCmd>( query_type, path_to_attribute, attribute, path_to_task);
}

const char*  QueryCmd::arg() { return CtsApi::queryArg();}

const char* QueryCmd::desc() {
   return
            "Query the status of attributes i.e state, event, meter, variable or trigger expression without blocking\n"
            " - state    return [unknown | complete | queued |             aborted | submitted | active] to standard out\n"
            " - dstate   return [unknown | complete | queued | suspended | aborted | submitted | active] to standard out\n"
            " - event    return 'set' | 'clear' to standard out\n"
            " - meter    return value of the meter to standard out\n"
            " - variable return value of the variable, repeat or generated variable to standard out,\n"
            "            will search up the node tree\n"
            " - trigger  returns 'true' if the expression is true, otherwise 'false'\n\n"
            "If this command is called within a '.ecf' script we will additionally log the task calling this command\n"
            "This is required to aid debugging for excessive use of this command\n"
            "The command will fail if the node path to the attribute does not exist in the definition and if:\n"
            " - event    The event is not found\n"
            " - meter    The meter is not found\n"
            " - variable No user or generated variable or repeat of that name found on node, or any of its parents\n"
            " - trigger  Trigger does not parse, or reference to nodes/attributes in the expression are not valid\n"
            "Arguments:\n"
            "  arg1 = [ state | event | meter | variable | trigger ]\n"
            "  arg2 = <path> | <path>:name where name is name of a event, meter or variable\n"
            "  arg3 = trigger expression\n\n"
            "Usage:\n"
            " ecflow_client --query state /path/to/node                         # return node state to standard out\n"
            " ecflow_client --query dstate /path/to/node                        # state that can included suspended\n"
            " ecflow_client --query event /path/to/task/with/event:event_name   # return set | clear to standard out\n"
            " ecflow_client --query meter /path/to/task/with/meter:meter_name   # returns the current value of the meter to standard out\n"
            " ecflow_client --query variable /path/to/task/with/var:var_name    # returns the variable value to standard out\n"
            " ecflow_client --query trigger /path/to/node/with/trigger \"/suite/task == complete\" # return true if expression evaluates false otherwise\n"
            ;
}

STC_Cmd_ptr QueryCmd::doHandleRequest(AbstractServer* as) const
{
   as->update_stats().query_++;

   node_ptr node = find_node(as,path_to_attribute_);

   if (query_type_ == "event") {
      const Event& event = node->findEventByNameOrNumber(attribute_);
      if (event.empty()) {
         std::stringstream ss; ss << "QueryCmd: Can not find event " << attribute_ << " on node " << path_to_attribute_;
         throw std::runtime_error(ss.str());
      }
      if (event.value()) return PreAllocatedReply::string_cmd( Event::SET());
      return PreAllocatedReply::string_cmd( Event::CLEAR());
   }

   if (query_type_ == "meter") {
      const Meter& meter = node->findMeter(attribute_);
      if (meter.empty()) {
         std::stringstream ss; ss << "QueryCmd: Can not find meter " << attribute_ << " on node " << path_to_attribute_;
         throw std::runtime_error(ss.str());
      }
      return PreAllocatedReply::string_cmd(boost::lexical_cast<std::string>(meter.value()));
   }

   if (query_type_ == "variable") {
      std::string the_value;
      if (node->findParentVariableValue(attribute_,the_value)) {
         return PreAllocatedReply::string_cmd( the_value );
      }
      std::stringstream ss; ss << "QueryCmd: Can not find variable, repeat or generated var' of name " << attribute_ << " on node " << path_to_attribute_ << " or its parents";
      throw std::runtime_error(ss.str());
   }

   if (query_type_ == "trigger") {
      std::unique_ptr<AstTop> ast = node->parse_and_check_expressions(attribute_,true,"QueryCmd:" ); // will throw for errors
      if ( ast->evaluate() ) return PreAllocatedReply::string_cmd("true");
      return PreAllocatedReply::string_cmd("false");
   }

   if (query_type_ == "state") {
      return PreAllocatedReply::string_cmd( NState::toString(node->state()) );
   }

   if (query_type_ == "dstate") {
      return PreAllocatedReply::string_cmd( DState::to_string(node->dstate()) );
   }
   else {
        std::stringstream ss;
        ss << "QueryCmd: unrecognised query_type " << query_type_  ;
        throw std::runtime_error( ss.str() ) ;
   }

   return PreAllocatedReply::ok_cmd();
}

std::ostream& operator<<(std::ostream& os, const QueryCmd& c) { return c.print(os); }
