/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
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
#include <boost/lexical_cast.hpp>

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

std::ostream& CheckPtCmd::print(std::ostream& os) const
{
   return user_cmd(os,CtsApi::checkPtDefs(mode_,check_pt_interval_,check_pt_save_time_alarm_));
}

bool CheckPtCmd::equals(ClientToServerCmd* rhs) const
{
   CheckPtCmd* the_rhs = dynamic_cast< CheckPtCmd* > ( rhs );
   if ( !the_rhs ) return false;
   if (mode_ != the_rhs->mode()) return false;
   if (check_pt_interval_ != the_rhs->check_pt_interval()) return false;
   if (check_pt_save_time_alarm_ != the_rhs->check_pt_save_time_alarm()) return false;
   return UserCmd::equals(rhs);
}

bool CheckPtCmd::isWrite() const
{
   // TODO: if save to takes to long, the the late flag is set. Even when command is read only ?
   if (mode_ != ecf::CheckPt::UNDEFINED) return true;
   if (check_pt_interval_ != 0) return true;
   if (check_pt_save_time_alarm_ != 0) return true;
   return false;
}

const char* CheckPtCmd::theArg() const
{
   return CtsApi::checkPtDefsArg();
}

STC_Cmd_ptr CheckPtCmd::doHandleRequest(AbstractServer* as) const
{
   // Placed here rather than in server. Since we want to record explicit request to check pt
   // The update_stats() is used to record the number of requests per second, hence we do not
   // want to skew this, and hence we ignore implicit request's via signal handling,
   // or when server terminates( does implicit check pt also)
   as->update_stats().checkpt_++;
   as->checkPtDefs(mode_,check_pt_interval_,check_pt_save_time_alarm_);
   return PreAllocatedReply::ok_cmd();
}

static const char* arg_desc()
{
            /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

   return
            "Forces the definition file in the server to be written to disk *or* allow mode,\n"
            "interval and alarm to be changed.\n"
            "Whenever the check pt file is written to disk, it is measured.\n"
            "If the time to save to disk is greater than the default of 30 seconds,\n"
            "then an alarm is raised. This can be seen in the GUI as a late flag on the server.\n"
            "Once the late flag has been set it will need to manually cleared in the GUI\n"
            "or by using --alter functionality\n"
            "Note excessive save times can interfere with job scheduling.\n"
            "The alarm threshold can be changed. See below.\n"
              "   arg1 = (optional) mode [ never | on_time | on_time:<integer> | always | <integer>]\n"
            "     never     : Never check point the definition in the server\n"
            "     on_time   : Turn on automatic check pointing at interval stored on server\n"
            "     on_time<integer> : Turn on automatic check point, with the specified interval in seconds\n"
            "     alarm<integer>   : Modify the alarm notification time for check pt saving to disk\n"
            "     always    : Check point at any change in node tree, *NOT* recommended for large definitions\n"
            "     <integer> : This specifies the interval in seconds when server should automatically check pt.\n"
            "                 This will only take effect of mode is on_time/CHECK_ON_TIME\n"
            "                 Should ideally be a value greater than 60 seconds, default is 120 seconds\n"
            "Usage:\n"
            "  --check_pt\n"
            "    Immediately check point the definition held in the server\n"
            "  --check_pt never\n"
            "    Switch off check pointing\n"
            "  --check_pt on_time\n"
            "    Start automatic check pointing at the interval stored in the server\n"
            "  --check_pt 180\n"
            "    Change the check pt interval to 180 seconds\n"
            "  --check_pt on_time:90\n"
            "    Change mode and interval, to automatic check pointing every 90 seconds\n"
            "  --check_pt alarm:35\n"
            "    Change the alarm time for check pt saves. i.e if saving the check pt takes longer than 35 seconds\n"
            "    set the late flag on the server."
            ;
}

void CheckPtCmd::addOption(boost::program_options::options_description& desc) const
{
   desc.add_options()(CtsApi::checkPtDefsArg(),po::value< string >()->implicit_value( string("") ),arg_desc());
}

static int parse_check_pt_interval( const std::string& the_arg)
{
   int check_pt_interval = 0;
   try { check_pt_interval = boost::lexical_cast<int>(the_arg); }
   catch (...) {
      std::stringstream ss;
      ss << "check_pt: Illegal argument(" << the_arg << "), expected [ never | on_time | on_time:<integer> | always | <integer>]\n" << arg_desc();
      throw std::runtime_error(ss.str());
   }
   if (check_pt_interval <= 0) {
       std::stringstream ss;
       ss << "check_pt: interval(" << check_pt_interval << ") must be greater than zero :\n" << arg_desc();
       throw std::runtime_error(ss.str());
   }
   return check_pt_interval;
}

static int parse_check_pt_alarm_time( const std::string& the_arg, int colon_pos)
{
   std::string alarm_time = the_arg.substr(colon_pos+1);

   int check_pt_alarm_time = 0;
   try { check_pt_alarm_time = boost::lexical_cast<int>(alarm_time); }
   catch (...) {
      std::stringstream ss;
      ss << "check_pt: Illegal argument(" << the_arg << "), expected [ never | on_time | on_time:<integer> | alarm::integer> | always | <integer>]\n" << arg_desc();
      throw std::runtime_error(ss.str());
   }
   if (check_pt_alarm_time <= 0) {
       std::stringstream ss;
       ss << "check_pt: alarm time(" << check_pt_alarm_time << ") must be greater than zero :\n" << arg_desc();
       throw std::runtime_error(ss.str());
   }
   return check_pt_alarm_time;
}


void CheckPtCmd::create(    Cmd_ptr& cmd,
         boost::program_options::variables_map& vm,
         AbstractClientEnv*  ace ) const
{
   if (ace->debug()) cout << "CheckPtCmd::create\n";

   std::string the_arg = vm[ theArg() ].as< std::string > ();

   if (ace->debug())  cout << "  CheckPtCmd::create arg = " << the_arg << "\n";

   ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED;
   int check_pt_interval = 0;
   int check_pt_save_time_alarm = 0;

   if (!the_arg.empty()) {
      size_t colon_pos = the_arg.find(":");
      if (colon_pos != std::string::npos) {
         // could be mode:interval or alarm:integer
         if (the_arg.find("alarm") != std::string::npos) {
            check_pt_save_time_alarm = parse_check_pt_alarm_time(the_arg,colon_pos);
         }
         else {
            std::string mode = the_arg.substr(0,colon_pos);
            std::string interval = the_arg.substr(colon_pos+1);

            if (mode == "never")        m = ecf::CheckPt::NEVER;
            else if (mode == "on_time") m = ecf::CheckPt::ON_TIME;
            else if (mode == "always")  m = ecf::CheckPt::ALWAYS;
            else {
               std::stringstream ss;
               ss << "check_pt: Illegal argument(" << the_arg << "), expected [ never | on_time | on_time:<integer> | alarm:<integer> | always | <integer>]\n" << arg_desc();
               throw std::runtime_error(ss.str());
            }
            check_pt_interval = parse_check_pt_interval(interval);
         }
      }
      else {
         if (the_arg == "never")        m = ecf::CheckPt::NEVER;
         else if (the_arg == "on_time") m = ecf::CheckPt::ON_TIME;
         else if (the_arg == "always")  m = ecf::CheckPt::ALWAYS;
         else {
            check_pt_interval = parse_check_pt_interval(the_arg);
         }
      }
   }

   // testing client interface
   if (ace->under_test())  return;

   if (ace->debug())  cout << "  CheckPtCmd::create mode = " << m << " check_pt_interval = " << check_pt_interval << "\n";

   cmd = Cmd_ptr(new CheckPtCmd(m,check_pt_interval,check_pt_save_time_alarm));
}

std::ostream& operator<<(std::ostream& os, const CheckPtCmd& c) { return c.print(os); }
