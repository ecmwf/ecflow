/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #62 $ 
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

#include <boost/lexical_cast.hpp>

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "ExprAst.hpp"
#include "SuiteChanged.hpp"
#include "Log.hpp"
#include "Extract.hpp"
#include "Limit.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

static std::string dump_args(const std::vector<std::string>& options, const std::vector<std::string>& paths )
{
   std::string the_args;
   for(const auto & option : options) {
      the_args += option;
      the_args += " ";
   }
   for(const auto & path : paths) {
      the_args += path;
      the_args += " ";
   }
   return the_args;
}

static AlterCmd::Delete_attr_type deleteAttrType(const std::string& s)
{
   if (s == "variable") return AlterCmd::DEL_VARIABLE;
   if (s == "time") return AlterCmd::DEL_TIME;
   if (s == "today") return AlterCmd::DEL_TODAY;
   if (s == "date") return AlterCmd::DEL_DATE;
   if (s == "day") return AlterCmd::DEL_DAY;
   if (s == "cron") return AlterCmd::DEL_CRON;
   if (s == "event") return AlterCmd::DEL_EVENT;
   if (s == "meter") return AlterCmd::DEL_METER;
   if (s == "label") return AlterCmd::DEL_LABEL;
   if (s == "trigger") return AlterCmd::DEL_TRIGGER;
   if (s == "complete") return AlterCmd::DEL_COMPLETE;
   if (s == "repeat") return AlterCmd::DEL_REPEAT;
   if (s == "limit") return AlterCmd::DEL_LIMIT;
   if (s == "limit_path") return AlterCmd::DEL_LIMIT_PATH;
   if (s == "inlimit") return AlterCmd::DEL_INLIMIT;
   if (s == "zombie") return AlterCmd::DEL_ZOMBIE;
   if (s == "late") return AlterCmd::DEL_LATE;
   if (s == "queue") return AlterCmd::DEL_QUEUE;
   if (s == "generic") return AlterCmd::DEL_GENERIC;
   return AlterCmd::DELETE_ATTR_ND;
}
static std::string to_string(AlterCmd::Delete_attr_type d)
{
   switch (d) {
   case AlterCmd::DEL_VARIABLE:  return "variable"; break;
   case AlterCmd::DEL_TIME:      return "time"; break;
   case AlterCmd::DEL_TODAY:     return "today"; break;
   case AlterCmd::DEL_DATE:      return "date"; break;
   case AlterCmd::DEL_DAY:       return "day"; break;
   case AlterCmd::DEL_CRON:      return "cron"; break;
   case AlterCmd::DEL_EVENT:     return "event"; break;
   case AlterCmd::DEL_METER:     return "meter"; break;
   case AlterCmd::DEL_LABEL:     return "label"; break;
   case AlterCmd::DEL_TRIGGER:   return "trigger"; break;
   case AlterCmd::DEL_COMPLETE:  return "complete";  break;
   case AlterCmd::DEL_REPEAT:    return "repeat";  break;
   case AlterCmd::DEL_LIMIT:     return "limit"; break;
   case AlterCmd::DEL_LIMIT_PATH:return "limit_path"; break;
   case AlterCmd::DEL_INLIMIT:   return "inlimit";  break;
   case AlterCmd::DEL_ZOMBIE:    return "zombie";  break;
   case AlterCmd::DEL_LATE:      return "late";  break;
   case AlterCmd::DEL_QUEUE:     return "queue";  break;
   case AlterCmd::DEL_GENERIC:   return "generic";  break;
   case AlterCmd::DELETE_ATTR_ND: break;
   default: break;
   }
   return string();
}
static void validDeleteAttr(std::vector<std::string>& vec)
{
   vec.reserve(19);
   vec.emplace_back("variable");
   vec.emplace_back("time");
   vec.emplace_back("today");
   vec.emplace_back("date");
   vec.emplace_back("day");
   vec.emplace_back("cron");
   vec.emplace_back("event");
   vec.emplace_back("meter");
   vec.emplace_back("label");
   vec.emplace_back("trigger");
   vec.emplace_back("complete");
   vec.emplace_back("repeat");
   vec.emplace_back("limit");
   vec.emplace_back("limit_path");
   vec.emplace_back("inlimit");
   vec.emplace_back("zombie");
   vec.emplace_back("late");
   vec.emplace_back("queue");
   vec.emplace_back("generic");
}


static AlterCmd::Add_attr_type addAttrType(const std::string& s)
{
   if (s == "time") return AlterCmd::ADD_TIME;
   if (s == "today") return AlterCmd::ADD_TODAY;
   if (s == "date") return AlterCmd::ADD_DATE;
   if (s == "day") return AlterCmd::ADD_DAY;
   if (s == "zombie") return AlterCmd::ADD_ZOMBIE;
   if (s == "variable") return AlterCmd::ADD_VARIABLE;
   if (s == "late") return AlterCmd::ADD_LATE;
   if (s == "limit") return AlterCmd::ADD_LIMIT;
   if (s == "inlimit") return AlterCmd::ADD_INLIMIT;
   if (s == "label") return AlterCmd::ADD_LABEL;
   return AlterCmd::ADD_ATTR_ND;
}
static std::string to_string(AlterCmd::Add_attr_type a) {
   switch (a) {
   case AlterCmd::ADD_TIME:    return "time;"; break;
   case AlterCmd::ADD_TODAY:   return "today"; break;
   case AlterCmd::ADD_DATE:    return "date"; break;
   case AlterCmd::ADD_DAY:     return "day"; break;
   case AlterCmd::ADD_ZOMBIE:  return "zombie"; break;
   case AlterCmd::ADD_VARIABLE:return "variable"; break;
   case AlterCmd::ADD_LATE:    return "late"; break;
   case AlterCmd::ADD_LIMIT:   return "limit"; break;
   case AlterCmd::ADD_INLIMIT: return "inlimit"; break;
   case AlterCmd::ADD_LABEL:   return "label"; break;
   case AlterCmd::ADD_ATTR_ND: break;
	}
	return string();
}
static void validAddAttr(std::vector<std::string>& vec)
{
   vec.reserve(10);
   vec.emplace_back("time");
   vec.emplace_back("today");
   vec.emplace_back("date");
   vec.emplace_back("day");
   vec.emplace_back("zombie");
   vec.emplace_back("variable");
   vec.emplace_back("late");
   vec.emplace_back("limit");
   vec.emplace_back("inlimit");
   vec.emplace_back("label");
}


static AlterCmd::Change_attr_type changeAttrType(const std::string& s)
{
   if (s == "variable") return AlterCmd::VARIABLE;
   if (s == "clock_type") return AlterCmd::CLOCK_TYPE;
   if (s == "clock_date") return AlterCmd::CLOCK_DATE;
   if (s == "clock_gain") return AlterCmd::CLOCK_GAIN;
   if (s == "clock_sync") return AlterCmd::CLOCK_SYNC;
   if (s == "event") return AlterCmd::EVENT;
   if (s == "meter") return AlterCmd::METER;
   if (s == "label") return AlterCmd::LABEL;
   if (s == "trigger") return AlterCmd::TRIGGER;
   if (s == "complete") return AlterCmd::COMPLETE;
   if (s == "repeat") return AlterCmd::REPEAT;
   if (s == "limit_max") return AlterCmd::LIMIT_MAX;
   if (s == "limit_value") return AlterCmd::LIMIT_VAL;
   if (s == "defstatus") return AlterCmd::DEFSTATUS;
   if (s == "late") return AlterCmd::LATE;
   return AlterCmd::CHANGE_ATTR_ND;
}
static std::string to_string(AlterCmd::Change_attr_type c)
{
   switch (c) {
   case AlterCmd::VARIABLE:     return "variable"; break;
   case AlterCmd::CLOCK_TYPE:   return "clock_type"; break;
   case AlterCmd::CLOCK_DATE:   return "clock_date"; break;
   case AlterCmd::CLOCK_GAIN:   return "clock_gain"; break;
   case AlterCmd::CLOCK_SYNC:   return "clock_sync"; break;
   case AlterCmd::EVENT:        return "event"; break;
   case AlterCmd::METER:        return "meter"; break;
   case AlterCmd::LABEL:        return "label"; break;
   case AlterCmd::TRIGGER:      return "trigger";  break;
   case AlterCmd::COMPLETE:     return "complete";  break;
   case AlterCmd::REPEAT:       return "repeat"; break;
   case AlterCmd::LIMIT_MAX:    return "limit_max"; break;
   case AlterCmd::LIMIT_VAL:    return "limit_value"; break;
   case AlterCmd::DEFSTATUS:    return "defstatus";  break;
   case AlterCmd::LATE:         return "late";  break;
   case AlterCmd::CHANGE_ATTR_ND: break;
   default: break;
   }
   return string();
}
static void validChangeAttr(std::vector<std::string>& vec)
{
   vec.reserve(16);
   vec.emplace_back("variable");
   vec.emplace_back("clock_type");
   vec.emplace_back("clock_gain");
   vec.emplace_back("clock_date");
   vec.emplace_back("clock_sync");
   vec.emplace_back("event");
   vec.emplace_back("meter");
   vec.emplace_back("label");
   vec.emplace_back("trigger");
   vec.emplace_back("complete");
   vec.emplace_back("repeat");
   vec.emplace_back("limit_max");
   vec.emplace_back("limit_value");
   vec.emplace_back("defstatus");
   vec.emplace_back("free_password");
   vec.emplace_back("late");
}

//=======================================================================================

bool AlterCmd::equals(ClientToServerCmd* rhs) const
{
   auto* the_rhs = dynamic_cast<AlterCmd*>(rhs);
   if (!the_rhs)  return false;
   if ( paths_             != the_rhs->paths())      { return false; }
   if ( name_              != the_rhs->name())       { return false; }
   if ( value_             != the_rhs->value())      { return false; }
   if ( del_attr_type_     != the_rhs->delete_attr_type()) { return false; }
   if ( change_attr_type_  != the_rhs->change_attr_type()) { return false; }
   if ( add_attr_type_     != the_rhs->add_attr_type())    { return false; }
   if ( flag_type_         != the_rhs->flag_type())        { return false; }
   if ( flag_              != the_rhs->flag())             { return false; }
   return UserCmd::equals(rhs);
}

void AlterCmd::alter_and_attr_type(std::string& alter_type,std::string& attr_type ) const
{
   if (del_attr_type_ != AlterCmd::DELETE_ATTR_ND) {
       alter_type = "delete";
       attr_type = to_string(del_attr_type_);
    }
    else if (change_attr_type_ != AlterCmd::CHANGE_ATTR_ND) {
       alter_type = "change";
       attr_type = to_string(change_attr_type_);
    }
    else if (add_attr_type_ != AlterCmd::ADD_ATTR_ND) {
       alter_type = "add";
       attr_type = to_string(add_attr_type_);
    }
    else if (flag_type_ != ecf::Flag::NOT_SET) {
       if (flag_) alter_type = "set_flag";
       else       alter_type = "clear_flag";
       attr_type = ecf::Flag::enum_to_string(flag_type_);
    }
    else {
       alter_type = "sort";
    }
}

std::ostream& AlterCmd::print_only(std::ostream& os) const
{
   std::string alter_type,attr_type;
   alter_and_attr_type(alter_type, attr_type);
   if (paths_.empty()) os << CtsApi::to_string(CtsApi::alter(std::vector<std::string>(1," "),alter_type,attr_type,name_,value_));
   else                os << CtsApi::to_string(CtsApi::alter(std::vector<std::string>(1,paths_[0]),alter_type,attr_type,name_,value_));
   return os;
}

std::ostream& AlterCmd::print(std::ostream& os) const
{
   std::string alter_type,attr_type;
   alter_and_attr_type(alter_type, attr_type);
   return user_cmd(os,CtsApi::to_string(CtsApi::alter(paths_,alter_type,attr_type,name_,value_)));
}

std::ostream& AlterCmd::print(std::ostream& os, const std::string& path) const
{
   std::string alter_type,attr_type;
   alter_and_attr_type(alter_type, attr_type);
   return user_cmd(os,CtsApi::to_string(CtsApi::alter(std::vector<std::string>(1,path),alter_type,attr_type,name_,value_)));
}


STC_Cmd_ptr AlterCmd::alter_server_state(AbstractServer* as) const
{
   if ( del_attr_type_ == AlterCmd::DEL_VARIABLE) {
      as->defs()->set_server().delete_user_variable(name_);
   }
   else if ( change_attr_type_ == AlterCmd::VARIABLE  || add_attr_type_ == AlterCmd::ADD_VARIABLE) {

      // ECFLOW-380: Some variable should be read only
      if (name_ == Str::ECF_HOST() || name_ == Str::ECF_PORT() || name_ == "ECF_PID" || name_ == "ECF_VERSION" || name_ == "ECF_LISTS" ) {
         std::stringstream ss; ss << "AlterCmd:: Can not add or change read only server variable " << name_;
         throw std::runtime_error(ss.str());
      }
      as->defs()->set_server().add_or_update_user_variables(name_,value_);
   }

   // Update defs flag state
   if (flag_type_ != Flag::NOT_SET) {
      if (flag_) as->defs()->flag().set(flag_type_);
      else       as->defs()->flag().clear(flag_type_);
   }

	// sort
	ecf::Attr::Type attr = Attr::to_attr(name_);
	if ( attr != ecf::Attr::UNKNOWN) {
	   bool recursive = (value_ == "recursive") ? true: false;
	   as->defs()->sort_attributes(attr,recursive);
	}

	return doJobSubmission( as );
}


STC_Cmd_ptr AlterCmd::doHandleRequest(AbstractServer* as) const
{
   as->update_stats().alter_cmd_++;

   std::stringstream ss;
   size_t vec_size = paths_.size();
   for(size_t i= 0; i < vec_size; i++) {

      /// For root node user means to change server state
      if (paths_[i] == "/") {
         return alter_server_state(as);
      }

      node_ptr node = find_node_for_edit_no_throw(as,paths_[i]);
      if (!node.get()) {
         ss << "AlterCmd: Could not find node at path " << paths_[i] << "\n";
         LOG(Log::ERR,"AlterCmd: Failed: Could not find node at path " << paths_[i]);
         continue;
      }

      SuiteChanged0 changed(node);
      try {
         switch (del_attr_type_) {
         case AlterCmd::DEL_VARIABLE:  node->deleteVariable(name_); break;
         case AlterCmd::DEL_TIME:      node->deleteTime(name_); break;
         case AlterCmd::DEL_TODAY:     node->deleteToday(name_); break;
         case AlterCmd::DEL_DATE:      node->deleteDate(name_); break;
         case AlterCmd::DEL_DAY:       node->deleteDay(name_);break;
         case AlterCmd::DEL_CRON:      node->deleteCron(name_);break;
         case AlterCmd::DEL_EVENT:     node->deleteEvent(name_);break;
         case AlterCmd::DEL_METER:     node->deleteMeter(name_); break;
         case AlterCmd::DEL_LABEL:     node->deleteLabel(name_); break;
         case AlterCmd::DEL_TRIGGER:   node->deleteTrigger();  break;
         case AlterCmd::DEL_COMPLETE:  node->deleteComplete();  break;
         case AlterCmd::DEL_REPEAT:    node->deleteRepeat(); break;
         case AlterCmd::DEL_LIMIT:     node->deleteLimit(name_);break;
         case AlterCmd::DEL_LIMIT_PATH:node->delete_limit_path(name_,value_);break;
         case AlterCmd::DEL_INLIMIT:   node->deleteInlimit(name_); break;
         case AlterCmd::DEL_ZOMBIE:    node->deleteZombie(name_); break;
         case AlterCmd::DEL_LATE:      node->deleteLate(); break;
         case AlterCmd::DEL_QUEUE:     node->delete_queue(name_); break;
         case AlterCmd::DEL_GENERIC:   node->delete_generic(name_); break;
         case AlterCmd::DELETE_ATTR_ND: break;
         default: break;
         }
      }
      catch ( std::exception& e) {
         ss << "Alter (delete) failed for " << paths_[i] << " : " << e.what() << "\n";
      }

      try {
         switch (change_attr_type_) {
         case AlterCmd::VARIABLE:    node->changeVariable(name_,value_); break;
         case AlterCmd::CLOCK_TYPE:  node->suite()->changeClockType(name_); break;  // node must be a suite, value must [hybrid|real| virtual]
         case AlterCmd::CLOCK_DATE:  node->suite()->changeClockDate(name_); break;  // Expecting day.month.year: node must be a suite, value must [hybrid|real| virtual]
         case AlterCmd::CLOCK_GAIN:  node->suite()->changeClockGain(name_); break;  // node must be a suite, value must be int
         case AlterCmd::CLOCK_SYNC:  node->suite()->changeClockSync(); break;       // node must be a suite, sync clock with computer
         case AlterCmd::EVENT:       node->changeEvent(name_,value_);break;  // if value is empty just set, [1|0] or name [set | clear]
         case AlterCmd::METER:       node->changeMeter(name_,value_); break;
         case AlterCmd::LABEL:       node->changeLabel(name_,value_); break;
         case AlterCmd::TRIGGER:     node->changeTrigger(name_);  break;          // expression must parse
         case AlterCmd::COMPLETE:    node->changeComplete(name_);  break;         // expression must parse
         case AlterCmd::REPEAT:      node->changeRepeat(name_); break;            //
         case AlterCmd::LIMIT_MAX:   node->changeLimitMax(name_,value_);break;    // value must be convertible to int
         case AlterCmd::LIMIT_VAL:   node->changeLimitValue(name_,value_); break; // value < limit max, & value must be convertible to an int
         case AlterCmd::DEFSTATUS:   node->changeDefstatus(name_);  break;        // must be a valid state
         case AlterCmd::LATE:        node->changeLate(LateAttr::create(name_)); break; // must be a valid late
         case AlterCmd::CHANGE_ATTR_ND: break;
         default: break;
         }
      }
      catch ( std::exception& e) {
         ss << "Alter (change) failed for " << paths_[i] << " : " << e.what() << "\n";
      }

		try {
			switch (add_attr_type_) {
			case AlterCmd::ADD_TIME:    node->addTime( TimeAttr(TimeSeries::create(name_ )) ); break;
			case AlterCmd::ADD_TODAY:   node->addToday( TodayAttr(TimeSeries::create(name_)) ); break;
			case AlterCmd::ADD_DATE:    node->addDate( DateAttr::create(name_) ); break;
			case AlterCmd::ADD_DAY:     node->addDay( DayAttr::create(name_) ); break;
			case AlterCmd::ADD_ZOMBIE:  node->addZombie( ZombieAttr::create(name_) ); break;
         case AlterCmd::ADD_VARIABLE:node->add_variable( name_, value_); break;
         case AlterCmd::ADD_LATE:    node->addLate( LateAttr::create(name_)); break;
         case AlterCmd::ADD_LABEL:   node->addLabel( Label(name_,value_) ); break;
         case AlterCmd::ADD_LIMIT: {
             int int_value = 0;
             try { int_value = boost::lexical_cast< int >( value_ ); }
             catch ( boost::bad_lexical_cast& ) {
                std::stringstream ss; ss << "AlterCmd: add_limit " << name_ << " " << value_ << " failed. Expected '" <<  value_ << "' to be convertible to an integer";
                throw std::runtime_error( ss.str() );
             }
            node->addLimit( Limit(name_, int_value));
            break;
         }
         case AlterCmd::ADD_INLIMIT: {
            string path_to_limit; // This can be empty
            string limitName;
            if ( !Extract::pathAndName( name_, path_to_limit, limitName ) ) {
               throw std::runtime_error( "AlterCmd::ADD_INLIMIT: Invalid inlimit : " + name_);
            }
            int token_value = 1;
            if (!value_.empty()) {
               try { token_value = boost::lexical_cast< int >( value_ ); }
               catch ( boost::bad_lexical_cast& ) {
                  ss << "AlterCmd: add_inlimit expected '" << value_ << "' to be convertible to an integer";
                  throw std::runtime_error( ss.str() );
               }
            }
            node->addInLimit( InLimit( limitName, path_to_limit, token_value ) ); // will throw if not valid
            break;
         }
         case AlterCmd::ADD_ATTR_ND:  break;
         }
      }
      catch ( std::exception& e) {
         ss << "Alter (add) failed for " << paths_[i] << ": Could not parse " << name_ << " : " << e.what() << "\n";
      }

      // Change flags
      if (flag_type_ != Flag::NOT_SET) {
         if (flag_) node->flag().set(flag_type_);
         else       node->flag().clear(flag_type_);
      }

      	// sort
	   ecf::Attr::Type attr = Attr::to_attr(name_);
	   if ( attr != ecf::Attr::UNKNOWN) {
	      bool recursive = (value_ == "recursive") ? true: false;
	      node->sort_attributes(attr,recursive);
	   }
   }

   std::string error_msg = ss.str();
   if (!error_msg.empty()) {
      throw std::runtime_error( error_msg ) ;
   }

   return doJobSubmission( as );
}

bool AlterCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const
{
   return do_authenticate(as,cmd,paths_);
}

const char* AlterCmd::arg()  { return CtsApi::alterArg();}
const char* AlterCmd::desc() {
            /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   return  "Alter the node according to the options.\n"
         "To add/delete/change server variables use '/' for the path.\n"
         "arg1 = [ delete | change | add | set_flag | clear_flag | sort ]\n"
         "         one option must be specified\n"
         "arg2 = For delete:\n"
         "         [ variable | time | today | date  | day | cron | event | meter | late | generic | queue |\n"
         "           label | trigger | complete | repeat | limit | inlimit | limit_path | zombie ]\n"
         "       For change:\n"
         "         [ variable | clock_type | clock_gain | clock_date | clock_sync  | event | meter | label |\n"
         "           trigger  | complete   | repeat     | limit_max  | limit_value | defstatus | late ]\n"
         "         *NOTE* If the clock is changed, then the suite will need to be re-queued in order for\n"
         "         the change to take effect fully.\n"
         "       For add:\n"
         "         [ variable | time | today | date | day | zombie | late | limit | inlimit | label ]\n"
         "       For set_flag and clear_flag:\n"
         "         [ force_aborted | user_edit | task_aborted | edit_failed |\n"
         "           ecfcmd_failed | no_script | killed | migrated | late |\n"
         "           message | complete | queue_limit | task_waiting | locked | zombie ]\n"
         "             archived | restored]\n"
         "       For sort:\n"
         "         [ event | meter | label | variable| limit | all ]\n"
         "arg3 = name/value\n"
         "       when changing, attributes like variable,meter,event,label,limits,late\n"
         "       we expect arguments to be quoted. For sort this argument can be called 'recursive'\n"
         "arg4 = new_value\n"
         "       specifies the new value only used for 'change'/'add'\n"
         "       values with spaces must be quoted\n"
         "arg5 = paths : At lease one node path required.The paths must start with a leading '/' character\n\n"
            "\nUsage:\n\n"
            "   ecflow_client --alter=add variable GLOBAL \"value\" /           # add server variable\n"
            "   ecflow_client --alter=add variable FRED \"value\" /path/to/node # add node variable\n"
            "   ecflow_client --alter=add time \"+00:20\" /path/to/node\n"
            "   ecflow_client --alter=add date \"01.*.*\" /path/to/node\n"
            "   ecflow_client --alter=add day \"sunday\"  /path/to/node\n"
            "   ecflow_client --alter=add label name \"label_value\" /path/to/node\n"
            "   ecflow_client --alter=add late \"-s 00:01 -a 14:30 -c +00:01\" /path/to/node\n"
            "   ecflow_client --alter=add limit mars \"100\" /path/to/node\n"
            "   ecflow_client --alter=add inlimit /path/to/node/withlimit:limit_name \"10\" /s1\n"
            "   # zombie attributes have the following structure:\n"
            "     `zombie_type`:(`client_side_action` | `server_side_action`):`child`:`zombie_life_time`\n"
            "      zombie_type        = \"user\" | \"ecf\" | \"path\"\n"
            "      client_side_action = \"fob\" | \"fail\" | \"block\"\n"
            "      server_side_action = \"adopt\" | \"delete\" | \"kill\"\n"
            "      child              = \"init\" | \"event\" | \"meter\" | \"label\" | \"wait\" | \"abort\" | \"complete\"\n"
            "      zombie_life_time   = unsigned integer default: user(300), ecf(3600), path(900)  minimum is 60\n"
            "   ecflow_client --alter=add zombie \"ecf:fail::\" /path/to/node     # ask system zombies to fail\n"
            "   ecflow_client --alter=add zombie \"user:fail::\" /path/to/node    # ask user generated zombies to fail\n"
            "   ecflow_client --alter=add zombie \"path:fail::\" /path/to/node    # ask path zombies to fail\n\n"
            "   ecflow_client --alter=delete variable FRED /path/to/node    # delete variable FRED\n"
            "   ecflow_client --alter=delete variable      /path/to/node    # delete *ALL* variables on the specified node\n"
            ;
}


void AlterCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( AlterCmd::arg(),po::value< vector<string> >()->multitoken(), AlterCmd::desc() );
}
void AlterCmd::create(  Cmd_ptr& cmd,
      boost::program_options::variables_map& vm,
      AbstractClientEnv* ac) const
{
	vector<string> args = vm[  arg() ].as< vector<string> >();

	if (ac->debug()) dumpVecArgs(AlterCmd::arg(),args);

	std::vector<std::string> options,paths;
	split_args_to_options_and_paths(args,options,paths); // relative order is still preserved
	if (paths.empty()) {
		std::stringstream ss;
		ss << "AlterCmd: No paths specified. Paths must begin with a leading '/' character\n" << dump_args(options,paths) << "\n";
		throw std::runtime_error( ss.str() );
	}
	if (options.empty()) {
		std::stringstream ss;
		ss << "AlterCmd: Invalid argument list:\n" << dump_args(options,paths)  << "\n";
		throw std::runtime_error( ss.str() );
	}
	if (options.size() < 2 ) {
		std::stringstream ss;
		ss << "Alter: At least three arguments expected. Found " << args.size() << "\n" << dump_args(options,paths) << "\n";
		throw std::runtime_error( ss.str() );
	}

	// arg[0] should one of [ add | delete | change | set_flag | clear_flag | sort ]
	std::string alterType = options[0];

	if ( alterType == "add") {

		createAdd(cmd,options,paths);
		return;
	}
	else if ( alterType == "change") {

		createChange(cmd,options,paths);
		return;
	}
	else if ( alterType == "delete") {

		createDelete(cmd,options,paths);
		return;
	}
	else if ( alterType == "set_flag") {

		create_flag(cmd,options,paths, true /*set */);
		return;
	}
	else if ( alterType == "clear_flag") {

		create_flag(cmd,options,paths, false /*clear */);
		return;
	}
   else if ( alterType == "sort") {

      create_sort_attributes(cmd,options,paths);
      return;
   }

	std::stringstream ss;
	ss << "Alter: The first argument must be one of [ change | delete | add | set_flag | clear_flag | sort ] but found '"
			<< alterType << "'\n" << dump_args(options,paths)  << "\n";
	throw std::runtime_error( ss.str() );
}

void AlterCmd::createAdd( Cmd_ptr& cmd, std::vector<std::string>& options, std::vector<std::string>& paths ) const
{
	// options[0]  - add
	// options[1]  - [ time | today | date | day | zombie | variable | late | limit | inlimit | label ]
	// options[2]  - [ time_string | date_string | day_string | zombie_string | variable_name | limit_name | path_to_limit ]
	// options[3]  - variable_value
	std::stringstream ss;

   AlterCmd::Add_attr_type theAttrType = addAttrType(options[1]);
   if (theAttrType == AlterCmd::ADD_ATTR_ND) {
      ss << "AlterCmd: add: The second argument must be one of [ ";
      std::vector<std::string> valid;
      validAddAttr(valid);
      for(size_t i = 0; i < valid.size(); ++i) {
         if (i != 0) ss << " | ";
         ss << valid[i];
      }
      ss << "] but found " << options[1] << "\n" << AlterCmd::desc();
      throw std::runtime_error( ss.str() );
   }

   if (options.size() < 3 ) {
      ss << "AlterCmd: add: At least four arguments expected. Found " << (options.size() + paths.size()) << "\n" << dump_args(options,paths) << "\n";
      throw std::runtime_error( ss.str() );
   }

   // **** parse and check format, expect this argument to be single or double tick quoted ****
   // **** for time,date,day or zombie
   std::string name = options[2];
   std::string value;
   try {
      switch (theAttrType) {
      case AlterCmd::ADD_TIME:  (void) TimeSeries::create(name); break;
      case AlterCmd::ADD_TODAY: (void) TimeSeries::create(name); break;
      case AlterCmd::ADD_DATE:  (void) DateAttr::create(name); break;
      case AlterCmd::ADD_DAY:   (void) DayAttr::create(name); break;
      case AlterCmd::ADD_ZOMBIE:(void) ZombieAttr::create(name); break;
      case AlterCmd::ADD_LATE:  (void) LateAttr::create(name); break;
		case AlterCmd::ADD_VARIABLE: {
			if (options.size() == 3 && paths.size() > 1) {
				// variable value may be a path, hence it will be in the paths parameter
				options.push_back(paths[0] );
				paths.erase( paths.begin() );
			}
			if (options.size() < 4 ) {
				ss << "AlterCmd: add: Expected 'add variable <name> <value> <paths>. Not enough arguments\n" << dump_args(options,paths) << "\n";
				throw std::runtime_error( ss.str() );
			}
			value = options[3];

			// Create a Variable to check valid names
			Variable check(name,value);
			break;
		}
      case AlterCmd::ADD_LABEL: {
         if (options.size() == 3 && paths.size() > 1) {
            // label value may be a path, hence it will be in the paths parameter
            options.push_back(paths[0] );
            paths.erase( paths.begin() );
         }
         if (options.size() < 4 ) {
            ss << "AlterCmd: add: Expected 'add label <name> <value> <paths>. Not enough arguments\n" << dump_args(options,paths) << "\n";
            throw std::runtime_error( ss.str() );
         }
         value = options[3];

         // Create a Label to check valid names
         Label check(name,value);
         break;
      }
      case AlterCmd::ADD_LIMIT: {
         if (options.size() < 4 ) {
            ss << "AlterCmd: add: Expected 'add limit <name> int. Not enough arguments\n" << dump_args(options,paths) << "\n";
            throw std::runtime_error( ss.str() );
         }
         value = options[3];

         int int_value = 0;
         try { int_value = boost::lexical_cast< int >( value ); }
         catch ( boost::bad_lexical_cast& ) {
            ss << "AlterCmd: " << options[0] << " " << options[1] << " " << options[2] << " " << options[3] << paths[0];
            ss << " expected '" << value << "' to be convertible to an integer\n";
            ss << dump_args(options,paths) << "\n";
            throw std::runtime_error( ss.str() );
         }
         Limit check(name,int_value); // will throw if not valid
         break;
      }
      case AlterCmd::ADD_INLIMIT: {  // inlimit /obs/limits:hpcd 2 name=hpcd, path=/obs/limits, tokens=2(optional)
         // options[0]  - add
         // options[1]  - [ inlimit ]
         // options[2]  - [ path_to_limit:limit_name ]
         // options[3]  - integer (optional)
         if (options.size() < 3 ) {
            ss << "AlterCmd: add: Expected 'add inlimit <path-to-limit:limit_name> <int>(optional) <paths>. Not enough arguments\n" << dump_args(options,paths) << "\n";
            throw std::runtime_error( ss.str() );
         }

         string path_to_limit; // This can be empty
         string limitName;
         if ( !Extract::pathAndName( options[2], path_to_limit, limitName ) ) {
            throw std::runtime_error( "AlterCmd::ADD_INLIMIT: Invalid inlimit : " +   options[2] );
         }

         int token_value = 1;
         if (options.size() == 4) {
            value = options[3];
            try { token_value = boost::lexical_cast< int >( options[3] ); }
            catch ( boost::bad_lexical_cast& ) {
               ss << "AlterCmd: " << options[0] << " " << options[1] << " " << options[2] << " " << options[3] << paths[0];
               ss << " expected '" << options[3] << "' to be convertible to an integer\n";
               ss << dump_args(options,paths) << "\n";
               throw std::runtime_error( ss.str() );
            }
         }
         InLimit inlimit( limitName, path_to_limit, token_value ); // will throw if not valid
         break;
      }
      case AlterCmd::ADD_ATTR_ND:break;
      }
   }
   catch ( std::exception& e) {
      ss << "AlterCmd: add: Could not parse " << name << ". Error: " << e.what()
            << "\n for time,today and date the new value should be a quoted string "
            << "\n for add expected: --alter add variable <name> <value> <paths>\n" << dump_args(options,paths) << "\n";
      throw std::runtime_error( ss.str() );
   }

   cmd = Cmd_ptr( new AlterCmd(paths,theAttrType, name, value) );
}


void AlterCmd::createDelete( Cmd_ptr& cmd, const std::vector<std::string>& options, const std::vector<std::string>& paths) const
{
   // options[0] = delete
   // options[1] = variable | time | today | date | day | cron | event | meter | label | trigger | complete | repeat | limit | limit_path | inlimit | zombie |late | generic | queue
   // options[2] = name ( of object to be delete ) optional
   // options[3] = limit_path (optional *ONLY* applicable for limit_path, specifies the path to be deleted
   AlterCmd::Delete_attr_type theAttrType = deleteAttrType(options[1]);
   if (theAttrType == AlterCmd::DELETE_ATTR_ND) {
      std::stringstream ss;
      ss << "Alter: delete: The second argument must be one of [ ";
      std::vector<std::string> valid;
      validDeleteAttr(valid);
      for(size_t i = 0; i < valid.size(); ++i) {
         if (i != 0) ss << " | ";
         ss << valid[i];
      }
      ss << "] but found " << options[1] << "\n" << AlterCmd::desc();
      throw std::runtime_error( ss.str() );
   }

   // Generally an empty third argument means delete all attributes, otherwise delete the specific one.
   std::string name;
   if (options.size() >= 3 )  name = options[2];

   // Deleting the limit path requires an additional arg
   std::string path_value;

   // if specified make sure its parses
   try {
      switch (theAttrType) {
      case AlterCmd::DEL_VARIABLE:  {
         if (!name.empty()) Variable check(name,""); // Create a Variable to check valid names
         break;
      }
      case AlterCmd::DEL_TIME: {
         if (!name.empty())  (void) TimeSeries::create(name) ; // will throw if not valid
         break;
      }
      case AlterCmd::DEL_TODAY: {
         if (!name.empty()) (void) TimeSeries::create(name) ; // will throw if not valid
         break;
      }
      case AlterCmd::DEL_DATE: {
         if (!name.empty()) (void) DateAttr::create(name); // will throw if not valid
         break;
      }
      case AlterCmd::DEL_DAY:  {
         if (!name.empty())  (void)DayAttr::create(name); // will throw if not valid
         break;
      }
      case AlterCmd::DEL_CRON:{
         if (!name.empty()) {
            CronAttr parsedCron = CronAttr::create(name); // will throw if not valid

            // additional check since parsing is very forgiving. if parsed string is same as default
            // then no cron was specified.
            CronAttr emptyCron;
            if ( emptyCron.structureEquals(parsedCron)) {
               throw std::runtime_error("Delete cron Attribute failed. Check cron " + name);
            }
         }
         break;
      }
      case AlterCmd::DEL_EVENT:    {
         if (!name.empty()) {
            Event check(name);  // will throw if not valid
         }
         break;
      }
      case AlterCmd::DEL_METER:    {
         if (!name.empty()) Meter check(name,0,100);  // will throw if not valid
         break;
      }
      case AlterCmd::DEL_LABEL:    {
         if (!name.empty()) Label check(name,"value");  // will throw if not valid
         break;
      }
      case AlterCmd::DEL_TRIGGER:  break; // there can only be one trigger per node, so we delete by path
      case AlterCmd::DEL_COMPLETE: break; // there can only be one complete per node, so we delete by path
      case AlterCmd::DEL_REPEAT:   break; // there can only be one repeat per node, so we delete by path
      case AlterCmd::DEL_LATE:     break; // there can only be one late per node, so we delete by path
      case AlterCmd::DEL_QUEUE:   {
         std::vector<std::string> vec;vec.emplace_back("a");
         if (!name.empty()) QueueAttr check(name,vec);  // will throw if not valid
         break;
      }
      case AlterCmd::DEL_GENERIC:   {
         if (!name.empty()) GenericAttr check(name);  // will throw if not valid
         break;
      }
      case AlterCmd::DEL_LIMIT:    {
         if (!name.empty()) Limit check(name,10);  // will throw if not valid
         break;
      }
      case AlterCmd::DEL_INLIMIT:  {
		   if (!name.empty()) {
		      // name can be:
		      //    limit_name
		      //    /path/to/limit:limit_name
		      string path_to_limit; // This can be empty
		      string limitName;
		      if ( !Extract::pathAndName( name, path_to_limit, limitName ) ) {
		         throw std::runtime_error( "AlterCmd::DEL_INLIMIT : Invalid inlimit : " +  name );
		      }
              InLimit check(limitName,path_to_limit);  // will throw if not valid
		   }
         break;
      }
      case AlterCmd::DEL_ZOMBIE:  {
         if (!Child::valid_zombie_type(name)) {
            throw std::runtime_error("Delete Zombie Attribute failed. Expected one of [ ecf | path | user ] but found " + name);
         }
         break;
      }
      case AlterCmd::DEL_LIMIT_PATH: {
         if (name.empty()) {
            std::stringstream ss;
            ss << "Delete limit_path failed. No limit name provided. Expected 5 args: delete limit_path <limit_name> <path-to-limit> <path_to_node>\n";
            ss << dump_args(options,paths) << "\n";
            throw std::runtime_error(ss.str());
         }

         std::vector<std::string> altered_path = paths;
         if (options.size() == 4) {
            // User has provide a limit path which does not start with '/'. Go with flow
            path_value = options[3];
         }
         else {
            // Since we have a limit path(i.e begins with'/') it will appear in the paths, as the first path
            if (paths.size() <= 1) {
               std::stringstream ss;
               ss << "Delete limit_path failed: No path to limit provided. Expected 5 args: delete limit_path <limit_name> <path-to-limit> <path_to_node>\n"
                     << dump_args(options,paths) << "\n";
               throw std::runtime_error(ss.str());
            }
            path_value = paths[0];

            // Change paths to remove the limit path.
            altered_path.erase(altered_path.begin());
         }
         cmd = Cmd_ptr( new AlterCmd(altered_path,theAttrType, name, path_value  ) );
         return;
      }
      case AlterCmd::DELETE_ATTR_ND: break;
      }
   }
   catch ( std::exception& e) {
      std::stringstream ss;
      ss << "AlterCmd: delete: Could not parse " << name << ". Error: " << e.what()
            << "\n for time,today and date the new value should be a quoted string\n" << dump_args(options,paths) << "\n";
      throw std::runtime_error( ss.str() );
   }

   cmd = Cmd_ptr( new AlterCmd(paths,theAttrType, name, path_value  ) );
}

void AlterCmd::createChange( Cmd_ptr& cmd, std::vector<std::string>& options, std::vector<std::string>& paths) const
{
   // options[0] = change
   // options[1] = variable | clock_type | clock_gain | clock_date | clock_sync | event | meter | label | trigger | complete | repeat | limit_max | limit_value | defstatus | late ]
   // options[2] = name
   // options[3] = value

   std::stringstream ss;

   AlterCmd::Change_attr_type theAttrType = changeAttrType(options[1]);
   if (theAttrType == AlterCmd::CHANGE_ATTR_ND) {
      ss << "AlterCmd: change: The third argument(" << options[1] << ") must be one of [ ";
      std::vector<std::string> valid;
      validChangeAttr(valid);
      for(size_t i = 0; i < valid.size(); ++i) {
         if (i != 0) ss << " | ";
         ss << valid[i];
      }
      ss << "]\n" <<  AlterCmd::desc();
      throw std::runtime_error( ss.str() );
   }

	std::string name, value;
	switch (theAttrType) {
	case AlterCmd::VARIABLE: {
		if (options.size() == 3 && paths.size() > 1) {
			// The variable value may be a path, and hence it will be paths and not options parameter
			options.push_back(paths[0]);
			paths.erase(paths.begin()); // remove first path, since it has been added to options
		}
      if (options.size() < 3 || options.size() > 4) {
         ss << "AlterCmd: change: expected 5 args : change variable <variable_name> <new_value> <path_to_node>";
         ss << " but found only " << (options.size() + paths.size()) << " arguments.\nThe value should be quoted if there are spaces\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
		name = options[2];
		if (options.size() == 4 ) value = options[3];
		break;}

   case AlterCmd::CLOCK_TYPE: {
      if (options.size() != 3) {
         ss << "AlterCmd: change: expected at least four args i.e. change clock_type [ hybrid | real ] <path_to_suite>";
         ss << " but found only " << (options.size() + paths.size()) << " arguments\n"
               << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      if (name != "hybrid" && name != "real") {
         ss << "AlterCmd: change clock_type: expected third argument to be one of [ hybrid | real ]";
         ss << " but found " << name << "\n" << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      break;}

   case AlterCmd::CLOCK_DATE: {
      if (options.size() != 3) {
         ss << "AlterCmd: change clock_date : expected at least four args :  change clock_date day.month.year <path_to_suite>";
         ss << " but found only " << (options.size() + paths.size()) << " arguments\n" << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];

      // Check date is in correct format:
      try {
         int day,month,year;
         DateAttr::getDate(name,day,month,year);
         DateAttr::checkDate(day,month,year,false /* for clocks we don't allow wild carding */);
      }
      catch ( std::exception& e) {
         ss << "AlterCmd:change  clock_date " << name << " is not valid. " << e.what();
         throw std::runtime_error( ss.str() );
      }
      break;}

   case AlterCmd::CLOCK_GAIN: {
      if (options.size() != 3) {
         ss << "AlterCmd: change clock_gain : expected four args i.e. change clock_gain <int> <path_to_suite> ";
         ss << " but found " << (options.size() + paths.size()) << " arguments. The actual gain must be convertible to an integer\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      try { boost::lexical_cast< int >( name ); }
      catch ( boost::bad_lexical_cast& ) {
         ss << "AlterCmd: " << options[0] << " " << options[1] << " " << options[2] << " " << paths[0];
         ss << " expected '" << name << "' to be convertible to an integer\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      break; }

   case AlterCmd::CLOCK_SYNC: {
      if (options.size() != 2) {
         ss << "AlterCmd: change clock_sync : expected three args i.e. change clock_sync  <path_to_suite> ";
         ss << " but found " << (options.size() + paths.size()) << " arguments.\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      break; }

   case AlterCmd::EVENT: {
      if (options.size() != 3 && options.size() != 4) {
         ss << "AlterCmd: Change event : expected four/five args:  change event <name_or_number> <[set | clear | <nothing>]> <path_to_node>";
         ss << " but found only " << (options.size() + paths.size()) << " arguments\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      if ( options.size() == 4) {
         value =  options[3];
         if (value != Event::SET() && value != Event::CLEAR()) {
            ss << "AlterCmd: Change event : expected four/five args:  change event <name_or_number> <[set | clear | <nothing>]> <path_to_node>";
            ss << " but found only " << (options.size() + paths.size()) << " arguments\n";
            ss << dump_args(options,paths) << "\n";
            throw std::runtime_error( ss.str() );
         }
      }
      // The name could be an integer
      try { boost::lexical_cast< int >( name ); }
      catch ( boost::bad_lexical_cast& ) {
         // name is not an integer, check name is valid
         Event check_name(name); // will throw if name is not valid
      }
      break; }

   case AlterCmd::METER: {
      if (options.size() != 4) {
         ss << "AlterCmd: change: expected five args: change meter meter_name meter_value  <path_to_node>";
         ss << " but found only " << (options.size() + paths.size()) << " arguments. The meter value must be convertible to an integer\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      Meter check(name,0,100); // Check meter name , by creating a meter

      value = options[3];
      try { boost::lexical_cast< int >( value );}
      catch ( boost::bad_lexical_cast& ) {
         ss << "AlterCmd: " << options[0] << " " << options[1] << " " << options[2] << " " << options[3] << " " << paths[0]
                                                                                                                         << " expected " << value << " to be convertible to an integer\n";
         throw std::runtime_error( ss.str() );
      }

      break; }

   case AlterCmd::LABEL: {
      if (options.size() == 3 && paths.size() == 1) {
         // ECFLOW-648 allow label value to be empty
         // HOWEVER , we can not cope multiple paths, and setting value to empty.
         // since empty quotes are removed by boost program options, hence if we have a label value which is path, and multiple paths
         value.clear();
      }
      else {
         // ECFLOW-480 take into account label values that is a path, adding quotes around the value does not help:
         // Note boost program options will remove the quotes around the value
         //      hence its difficult to say what is an option and what is a path.
         //      However since we expect 4(change,label,<label_name>,<label_value>) options, work around the problem
         if (options.size() == 3  && paths.size() > 1) {
            options.push_back(paths[0]);
            paths.erase(paths.begin());  // remove first path, since it has been added to options
         }
         if (options.size() != 4) {
            ss << "AlterCmd: change label expected at least five args : change label <label_name> <label_value> <path_to_node> ";
            ss << " but found  " << (options.size() + paths.size()) << " arguments. the label value should be quoted\n";
            ss << dump_args(options,paths) << "\n";
            throw std::runtime_error( ss.str() );
         }
         value = options[3];
      }
      name = options[2];

      Label check(name,value); // Check name , by creating
      break; }

   case AlterCmd::LATE: {
      if (options.size() != 3) {
         ss << "AlterCmd: change: expected three args: change late \"late -s +00:15  -a  20:00  -c +02:00\" <path_to_node>";
         ss << " but found only " << (options.size() + paths.size()) << " arguments\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      (void) LateAttr::create(name); // Check we can create the late
      break; }

	case AlterCmd::TRIGGER: {
	   if (options.size() != 3) {
	      ss << "AlterCmd: change: expected four args : change trigger 'expression' <path_to_node>";
	      ss << " but found " << (options.size() + paths.size()) << " arguments. The trigger expression must be quoted\n";
	      ss << dump_args(options,paths) << "\n";
	      throw std::runtime_error( ss.str() );
	   }
	   name = options[2];

	   std::string error_msg = "AlterCmd: change trigger:";
	   std::unique_ptr<AstTop> ast = Expression::parse_no_throw(name,error_msg);
	   if (!ast.get()) {
	      ss << error_msg << "\n" << dump_args(options,paths) << "\n";
	      throw std::runtime_error( ss.str() );
	   }
		break; }

   case AlterCmd::COMPLETE: {
      if (options.size() != 3) {
         ss << "AlterCmd: change complete: expected four args: change complete 'expression'  <path_to_node> ";
         ss << " but found " << (options.size() + paths.size()) << " arguments. The expression must be quoted\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];

	   std::string error_msg = "AlterCmd: change complete:";
	   std::unique_ptr<AstTop> ast = Expression::parse_no_throw(name,error_msg);
	   if (!ast.get()) {
	      ss << error_msg << "\n" << dump_args(options,paths) << "\n";
	      throw std::runtime_error( ss.str() );
	   }
		break;}

   case AlterCmd::REPEAT: {
      // *NOTE* a Node can only have *ONE* repeat, hence no need to provide name
      if (options.size() != 3) {
         ss << "AlterCmd: change repeat: expected four arg's : change repeat [ integer | string ] <path_to_node>";
         ss << " but found only " << (options.size() + paths.size()) << " arguments.\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      break; }

   case AlterCmd::LIMIT_MAX: {
      if (options.size() != 4) {
         ss << "AlterCmd: change: limit_max: : expected five arguments : change limit_max <limit_name> <int> <path_to_node>";
         ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      value = options[3];
      int limit = 0;
      try { limit = boost::lexical_cast< int >( value );}
      catch ( boost::bad_lexical_cast& ) {
         ss << "AlterCmd: change: limit-max: " << options[0] << " " << options[1] << " " << options[2] << " " << options[3] << " " << paths[0]
                                                                                                                                            << " expected " <<  value << " to be convertible to an integer\n";
         throw std::runtime_error( ss.str() );
      }
      Limit check(name,limit); // Check name , by creating
      break; }

   case AlterCmd::LIMIT_VAL: {
      if (options.size() != 4) {
         ss << "AlterCmd: change: limit-value: expected five arguments : change limit_value <limit_name> <int> <path_to_node>";
         ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      value = options[3];
      try { boost::lexical_cast< int >( value );}
      catch ( boost::bad_lexical_cast& ) {
         ss << "AlterCmd: change: limit_value: " << options[0] << " " << options[1] << " " << options[2] << " " << options[3] << " " << paths[0]
                                                                                                                                              << " expected " <<  value << " to be convertible to an integer\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      Limit check(name,10); // Check name, by creating
      break;}

   case AlterCmd::DEFSTATUS: {
      if (options.size() != 3) {
         ss << "AlterCmd: change defstatus expected four args : change defstatus [ queued | complete | unknown | aborted | suspended ] <path_to_node>";
         ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
         ss << dump_args(options,paths) << "\n";
         throw std::runtime_error( ss.str() );
      }
      name = options[2];
      if (!DState::isValid(name)) {
         ss << "AlterCmd: " << options[0] << " " << options[1] << " " << options[2] << " " << paths[0]
                                                                                                    << "an expected " <<  name << " to be a valid state,  i.e one of [ queued | complete | unknown | aborted | suspended ]\n";
         throw std::runtime_error( ss.str() );
      }
      break; }

   case AlterCmd::CHANGE_ATTR_ND:    break;
   default: break;
   }

   cmd = Cmd_ptr( new AlterCmd(paths,theAttrType, name, value  ) );
}

void AlterCmd::create_flag( Cmd_ptr& cmd, const std::vector<std::string>& options, const std::vector<std::string>& paths, bool flag) const
{
   // options[0] = set_flag | clear_flag
   // options[1] = [ force_aborted ....]

   Flag::Type theFlagType = Flag::string_to_flag_type(options[1]);
   if (theFlagType == Flag::NOT_SET) {
      std::stringstream ss;
      ss << "AlterCmd: set/clear_flag: The second argument(" << options[1] << ") must be one of [ ";
      std::vector<std::string> valid;
      Flag::valid_flag_type(valid);
      for(size_t i = 0; i < valid.size(); ++i) { if (i != 0) ss << " | "; ss << valid[i]; }
      ss << "]\n" <<  AlterCmd::desc();
      throw std::runtime_error( ss.str() );
   }

   cmd = Cmd_ptr( new AlterCmd(paths,theFlagType, flag  ) );
}

void AlterCmd::create_sort_attributes(Cmd_ptr& cmd,const std::vector<std::string>& options,const std::vector<std::string>& paths) const
{
   // options[0]  - sort
   // options[1]  - [ event | meter | label | limit | variable | all ]
   // options[2]  - recursive
   std::stringstream ss;

   if (options.size() < 2 ) {
      ss << "AlterCmd: add: At least three arguments expected. Found " << (options.size() + paths.size()) << "\n" << dump_args(options,paths) << "\n";
      throw std::runtime_error( ss.str() );
   }

   ecf::Attr::Type theAttrType = Attr::to_attr(options[1]);
   if (theAttrType == Attr::UNKNOWN) {
      ss << "AlterCmd: sort: The second argument must be one of [ ";
      std::vector<std::string> valid = Attr::all_attrs();
      for(size_t i = 0; i < valid.size(); ++i) {
         if (i != 0) ss << " | ";
         ss << valid[i];
      }
      ss << "] but found " << options[1] << "\n" << AlterCmd::desc();
      throw std::runtime_error( ss.str() );
   }
   std::string name = options[1];

   std::string value;
   if (options.size() == 3) {
      if (options[2] != "recursive") {
         ss << "AlterCmd: sort: Expected third argument to be 'recursive' but found '" << options[2] << "\n" << AlterCmd::desc();
         throw std::runtime_error( ss.str() );
      }
      value = "recursive";
   }

   cmd = Cmd_ptr( new AlterCmd(paths, name,value) );
}

std::ostream& operator<<(std::ostream& os, const AlterCmd& c) { return c.print(os); }
