//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #35 $ 
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

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include "Stats.hpp"
#include "SState.hpp"

using namespace std;

Stats::Stats() :

	status_(0),
	request_count_(0),
	job_sub_interval_(0),
   checkpt_interval_(0),
   checkpt_save_time_alarm_(0),
   checkpt_mode_(ecf::CheckPt::UNDEFINED),
	no_of_suites_(0),

	checkpt_(0),
	restore_defs_from_checkpt_(0),

   server_version_(0),
   restart_server_(0),
	shutdown_server_(0),
	halt_server_(0),
 	reload_white_list_file_(0),
 	reload_passwd_file_(0),
   ping_(0),
   debug_server_on_(0),
   debug_server_off_(0),
	get_defs_(0),
   sync_(0),
   sync_full_(0),
   sync_clock_(0),
	news_(0),

   node_job_gen_(0),
   node_check_job_gen_only_(0),
	node_delete_(0),
	node_suspend_(0),
	node_resume_(0),
	node_kill_(0),
	node_status_(0),
	node_edit_history_(0),
   node_archive_(0),
   node_restore_(0),


	log_cmd_(0),
	log_msg_cmd_(0),
	begin_cmd_(0),

	task_init_(0),
	task_complete_(0),
	task_wait_(0),
	task_abort_(0),
	task_event_(0),
	task_meter_(0),
   task_label_(0),
   task_queue_(0),

	zombie_fob_(0),
	zombie_fail_(0),
	zombie_adopt_(0),
	zombie_remove_(0),
	zombie_get_(0),
   zombie_block_(0),
   zombie_kill_(0),

	requeue_node_(0),
	order_node_(0),
	run_node_(0),
 	load_defs_(0),
	replace_(0),
	force_(0),
	free_dep_(0),
	suites_(0),
	edit_script_(0),

   alter_cmd_(0),
   ch_cmd_(0),
	file_ecf_(0),
	file_job_(0),
   file_jobout_(0),
   file_cmdout_(0),
	file_manual_(0),

	plug_(0),
	move_(0),
   group_cmd_(0),
   server_load_cmd_(0),
   stats_(0),
   check_(0),
   query_(0)
{
}

void Stats::update_stats(int poll_interval)
{
   // Called at poll time, ie just before node tree traversal
   request_vec_.emplace_back(request_count_,poll_interval );
   request_count_ = 0;
   request_stats_.clear();

   // To stop excessive memory usage , we will only store, request per poll period, for 1 hour
   if (request_vec_.size() > 60) {
      request_vec_.pop_front();
   }
}

void Stats::update_for_serialisation()
{
   /// This *ONLY* compute the data when this function is called
   /// >>> Hence the server load is only valid for last hour <<<
   no_of_suites_ = 0;
   if (request_vec_.empty()) return;

   std::stringstream ss;
   int count = 0;
   int request = 0;
   int seconds = 0;
   auto rend = request_vec_.rend();
   for(auto i = request_vec_.rbegin(); i != rend; ++i) {
      count++;
      request += (*i).first;
      seconds += (*i).second;
      double request_per_second = (double)request/seconds;

      if (count == 1) {
         ss << setiosflags(ios::fixed) << setprecision(2) << request_per_second;
      }
      else if (count ==  5) {
         ss << " " << request_per_second;
      }
      else if (count == 15) {
         ss << " " << request_per_second;
      }
      else if (count == 30) {
         ss << " " << request_per_second ;
      }
      else if (count == 60) {
         ss << " " << request_per_second;
      }
   }
   request_stats_ = ss.str();
}

void Stats::reset()
{
   checkpt_ = 0;
   restore_defs_from_checkpt_ = 0;

   server_version_ = 0;
   restart_server_ = 0;
   shutdown_server_ = 0;
   halt_server_ = 0;
   reload_white_list_file_ = 0;
   reload_passwd_file_ = 0;
   ping_ = 0;
   debug_server_on_ = 0;
   debug_server_off_ = 0;
   get_defs_ = 0;
   sync_ = 0;
   sync_full_ = 0;
   sync_clock_ = 0;
   news_ = 0;

   node_job_gen_ = 0;
   node_check_job_gen_only_ = 0;
   node_delete_ = 0;
   node_suspend_ = 0;
   node_resume_ = 0;
   node_kill_ = 0;
   node_status_ = 0;
   node_edit_history_ = 0;
   node_archive_ = 0;
   node_restore_ = 0;


   log_cmd_ = 0;
   log_msg_cmd_ = 0;
   begin_cmd_ = 0;

   task_init_ = 0;
   task_complete_ = 0;
   task_wait_ = 0;
   task_abort_ = 0;
   task_event_ = 0;
   task_meter_ = 0;
   task_label_ = 0;
   task_queue_ = 0;

   zombie_fob_ = 0;
   zombie_fail_ = 0;
   zombie_adopt_ = 0;
   zombie_remove_ = 0;
   zombie_get_ = 0;
   zombie_block_ = 0;
   zombie_kill_ = 0;

   requeue_node_ = 0;
   order_node_ = 0;
   run_node_ = 0;
   load_defs_ = 0;
   replace_ = 0;
   force_ = 0;
   free_dep_ = 0;
   suites_ = 0;
   edit_script_ = 0;

   alter_cmd_ = 0;
   ch_cmd_ = 0;
   file_ecf_ = 0;
   file_job_ = 0;
   file_jobout_ = 0;
   file_cmdout_ = 0;
   file_manual_ = 0;

   plug_ = 0;
   move_ = 0;
   group_cmd_ = 0;
   server_load_cmd_ = 0;
   stats_ = 0;
   check_ = 0;
   query_ = 0;
}

static string show_checkpt_mode(ecf::CheckPt::Mode m){
   switch (m) {
      case ecf::CheckPt::NEVER: return "CHECK_NEVER"; break;
      case ecf::CheckPt::ON_TIME: return "CHECK_ON_TIME"; break;
      case ecf::CheckPt::ALWAYS: return "CHECK_ON_ALWAYS"; break;
      case ecf::CheckPt::UNDEFINED: return "UNDEFINED"; break;
   }
   return std::string();
}

void Stats::show(std::ostream& os) const
{
   int width = 35;
   os << "Server statistics\n";
   os << left << setw(width) << "   Version " << version_ << "\n";
   os << left << setw(width) << "   Status " << SState::to_string(status_) << "\n";
   os << left << setw(width) << "   Host " << host_ << "\n";
   os << left << setw(width) << "   Port " << port_ << "\n";
   os << left << setw(width) << "   Up since " << up_since_ << "\n";
   os << left << setw(width) << "   Job sub' interval " << job_sub_interval_ << "s\n";
   os << left << setw(width) << "   ECF_HOME " <<   ECF_HOME_ << "\n";
   os << left << setw(width) << "   ECF_LOG " <<   ECF_LOG_ << "\n";
   os << left << setw(width) << "   ECF_CHECK " <<   ECF_CHECK_ << "\n";
   os << left << setw(width) << "   Check pt interval " << checkpt_interval_ << "s\n";
   os << left << setw(width) << "   Check pt mode " << show_checkpt_mode(checkpt_mode_) << "\n";
   os << left << setw(width) << "   Check pt save time alarm " << checkpt_save_time_alarm_ << "s\n";
   os << left << setw(width) << "   Number of Suites " << no_of_suites_ << "\n";
   os << left << setw(width) << "   Request's per 1,5,15,30,60 min " <<  request_stats_ << "\n";

   if (checkpt_ || restore_defs_from_checkpt_ || server_version_ || restart_server_ || shutdown_server_ || halt_server_ || 
       ping_ || debug_server_on_ || debug_server_off_ || get_defs_ || sync_ || sync_full_ || sync_clock_ || news_)  os << "\n";
   if (!locked_by_user_.empty()) os << left << setw(width) << "   Locked by user " << locked_by_user_ << "\n";
   if (checkpt_ != 0)                   os << left << setw(width) << "   Check points " << checkpt_ << "\n";
   if (restore_defs_from_checkpt_ != 0) os << left << setw(width) << "   Restore from Check point " << restore_defs_from_checkpt_ << "\n";
   if (restart_server_ != 0)   os << left << setw(width) << "   Restart server " << restart_server_ << "\n";
   if (shutdown_server_ != 0)  os << left << setw(width) << "   Shutdown server " << shutdown_server_ << "\n";
   if (halt_server_ != 0)      os << left << setw(width) << "   Halt server " << halt_server_ << "\n";
   if (ping_ != 0)             os << left << setw(width) << "   Ping " << ping_ << "\n";
   if (debug_server_on_ != 0)  os << left << setw(width) << "   debug server on " << debug_server_on_ << "\n";
   if (debug_server_off_ != 0) os << left << setw(width) << "   debug server off " << debug_server_off_ << "\n";
   if (get_defs_ != 0)         os << left << setw(width) << "   Get full definition " << get_defs_ << "\n";
   if (server_version_ != 0)   os << left << setw(width) << "   Server version " << server_version_ << "\n";
   if (sync_ != 0)             os << left << setw(width) << "   Sync " << sync_ << "\n";
   if (sync_full_ != 0)        os << left << setw(width) << "   Sync full " << sync_full_ << "\n";
   if (sync_clock_ != 0)       os << left << setw(width) << "   Sync suite clock " << sync_clock_ << "\n";
   if (news_ != 0)             os << left << setw(width) << "   News " << news_ << "\n";
   
   if (task_init_ || task_complete_ || task_wait_ || task_abort_ || task_event_ || task_meter_ || task_label_ || task_queue_)  os << "\n";
   if (task_init_ != 0)     os << left << setw(width) << "   Task init " << task_init_ << "\n";
   if (task_complete_ != 0) os << left << setw(width) << "   Task complete " << task_complete_ << "\n";
   if (task_wait_ != 0)     os << left << setw(width) << "   Task wait " << task_wait_ << "\n";
   if (task_abort_ != 0)    os << left << setw(width) << "   Task abort " << task_abort_ << "\n";
   if (task_event_ != 0)    os << left << setw(width) << "   Task event " << task_event_ << "\n";
   if (task_meter_ != 0)    os << left << setw(width) << "   Task meter " << task_meter_ << "\n";
   if (task_label_ != 0)    os << left << setw(width) << "   Task label " << task_label_ << "\n";
   if (task_queue_ != 0)    os << left << setw(width) << "   Task queue " << task_queue_ << "\n";
   
   if (zombie_fob_ || zombie_fail_ || zombie_adopt_ || zombie_remove_ || zombie_get_ || zombie_block_ || zombie_kill_)  os << "\n";
   if (zombie_fob_ != 0)     os << left << setw(width) << "   Zombie fob " << zombie_fob_ << "\n";
   if (zombie_fail_ != 0)    os << left << setw(width) << "   Zombie fail " << zombie_fail_ << "\n";
   if (zombie_adopt_ != 0)   os << left << setw(width) << "   Zombie adopt " << zombie_adopt_ << "\n";
   if (zombie_remove_ != 0)  os << left << setw(width) << "   Zombie remove " << zombie_remove_ << "\n";
   if (zombie_get_ != 0)     os << left << setw(width) << "   Zombie get " << zombie_get_ << "\n";
   if (zombie_block_ != 0)   os << left << setw(width) << "   Zombie block " << zombie_block_ << "\n";
   if (zombie_kill_ != 0)    os << left << setw(width) << "   Zombie kill " << zombie_kill_ << "\n";
   
   if (load_defs_ || begin_cmd_ || requeue_node_ || node_job_gen_ || node_check_job_gen_only_ || node_delete_ || node_suspend_ ||
       node_resume_ || node_kill_ || node_status_ || node_edit_history_ || log_cmd_ || log_msg_cmd_ || order_node_ || run_node_ || replace_ ||
       force_ || free_dep_ || suites_ || edit_script_ || alter_cmd_ || ch_cmd_ || plug_ || move_ || group_cmd_ ||
       reload_white_list_file_ || server_load_cmd_ ||  stats_ || check_ || query_
       || reload_passwd_file_ || node_archive_ || node_restore_
       )  os << "\n";

   if (load_defs_ != 0)         os << left << setw(width) << "   Load definition " << load_defs_ << "\n";
   if (begin_cmd_ != 0)         os << left << setw(width) << "   Begin " << begin_cmd_ << "\n";
   if (requeue_node_ != 0)      os << left << setw(width) << "   Requeue " << requeue_node_ << "\n";
   if (node_job_gen_ != 0)      os << left << setw(width) << "   Job generation " << node_job_gen_ << "\n";
   if (node_check_job_gen_only_ != 0) os << left << setw(width) << "   Check Job generation " << node_check_job_gen_only_ << "\n";
   if (node_delete_ != 0)       os << left << setw(width) << "   Node delete " << node_delete_ << "\n";
   if (node_suspend_ != 0)      os << left << setw(width) << "   Node suspend " << node_suspend_ << "\n";
   if (node_resume_ != 0)       os << left << setw(width) << "   Node resume " << node_resume_ << "\n";
   if (node_kill_ != 0)         os << left << setw(width) << "   Node kill " << node_kill_ << "\n";
   if (node_status_ != 0)       os << left << setw(width) << "   Node status " << node_status_ << "\n";
   if (node_edit_history_ != 0) os << left << setw(width) << "   Node edit history " << node_edit_history_ << "\n";
   if (node_archive_ != 0)      os << left << setw(width) << "   Node archive " << node_archive_ << "\n";
   if (node_restore_ != 0)      os << left << setw(width) << "   Node restore " << node_restore_ << "\n";
   if (log_cmd_ != 0)           os << left << setw(width) << "   Log cmd " << log_cmd_ << "\n";
   if (log_msg_cmd_ != 0)       os << left << setw(width) << "   Log message " << log_msg_cmd_ << "\n";
   if (order_node_ != 0)        os << left << setw(width) << "   Order " << order_node_ << "\n";
   if (run_node_ != 0)          os << left << setw(width) << "   Run " << run_node_ << "\n";
   if (replace_ != 0)           os << left << setw(width) << "   Replace " << replace_ << "\n";
   if (force_ != 0)             os << left << setw(width) << "   Force  " << force_ << "\n";
   if (free_dep_ != 0)          os << left << setw(width) << "   Free dependencies " << free_dep_ << "\n";
   if (suites_ != 0)            os << left << setw(width) << "   Suites " << suites_ << "\n";
   if (edit_script_ != 0)       os << left << setw(width) << "   Edit script " << edit_script_ << "\n";
   if (alter_cmd_ != 0)         os << left << setw(width) << "   Alter " << alter_cmd_ << "\n";
   if (ch_cmd_ != 0)            os << left << setw(width) << "   Client handle " << ch_cmd_ << "\n";
   if (plug_ != 0)              os << left << setw(width) << "   Plug " << plug_ << "\n";
   if (move_ != 0)              os << left << setw(width) << "   Move " << move_ << "\n";
   if (group_cmd_ != 0)         os << left << setw(width) << "   Group " << group_cmd_ << "\n";
   if (server_load_cmd_ != 0)   os << left << setw(width) << "   Server load cmd " << server_load_cmd_ << "\n";
   if (stats_ != 0)             os << left << setw(width) << "   stats cmd " << stats_ << "\n";
   if (check_ != 0)             os << left << setw(width) << "   checks " << check_ << "\n";
   if (query_ != 0)             os << left << setw(width) << "   query " << query_ << "\n";
   if (reload_white_list_file_ != 0) os << left << setw(width) << "   Reload white list file " << reload_white_list_file_ << "\n";
   if (reload_passwd_file_ != 0)     os << left << setw(width) << "   Reload password file " << reload_passwd_file_ << "\n";
   if (file_ecf_ || file_job_ || file_jobout_ || file_manual_ || file_cmdout_)  os << "\n";
   if (file_ecf_ != 0)    os << left << setw(width) << "   File ECF " << file_ecf_ << "\n";
   if (file_job_ != 0) os << left << setw(width) << "   File job " << file_job_ << "\n";
   if (file_jobout_ != 0) os << left << setw(width) << "   File Job out " << file_jobout_ << "\n";
   if (file_cmdout_ != 0) os << left << setw(width) << "   File Cmd out " << file_cmdout_ << "\n";
   if (file_manual_ != 0) os << left << setw(width) << "   File manual " << file_manual_ << "\n";
   os << flush;
}
