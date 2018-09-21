#ifndef STATS_HPP_
#define STATS_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #31 $ 
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

#include <deque>
#include <sstream>
#include "CheckPt.hpp"
#include "Serialization.hpp"


/// This class is used to store all statistical data about all the
/// commands processed by the server. Uses default copy constructor
struct Stats {

   Stats();
   void show(std::ostream& os = std::cout) const;

   void update() { request_count_++; }
   void update_stats(int poll_interval);
   void update_for_serialisation();
   void reset();

	int status_{0};    // 0 HALTED, 1 SHUTDOWN, 2 RUNNING
	std::string locked_by_user_;
	std::string host_;
	std::string port_;
	std::string up_since_;
	std::string version_;
	std::string request_stats_;
   std::string ECF_HOME_;
   std::string ECF_CHECK_;
   std::string ECF_LOG_;

	int request_count_{0};
	int job_sub_interval_{0};
   int checkpt_interval_{0};
   int checkpt_save_time_alarm_{0};
   ecf::CheckPt::Mode checkpt_mode_{ecf::CheckPt::UNDEFINED};
	int no_of_suites_{0};

	unsigned int checkpt_{0};
	unsigned int restore_defs_from_checkpt_{0};

   unsigned int server_version_{0};
   unsigned int restart_server_{0};
	unsigned int shutdown_server_{0};
	unsigned int halt_server_{0};
 	unsigned int reload_white_list_file_{0};
 	unsigned int reload_passwd_file_{0};
   unsigned int ping_{0};
   unsigned int debug_server_on_{0};
   unsigned int debug_server_off_{0};
	unsigned int get_defs_{0};
   unsigned int sync_{0};
   unsigned int sync_full_{0};
   unsigned int sync_clock_{0};
	unsigned int news_{0};

   unsigned int node_job_gen_{0};
   unsigned int node_check_job_gen_only_{0};
	unsigned int node_delete_{0};
	unsigned int node_suspend_{0};
	unsigned int node_resume_{0};
	unsigned int node_kill_{0};
   unsigned int node_status_{0};
   unsigned int node_edit_history_{0};
   unsigned int node_archive_{0};
   unsigned int node_restore_{0};

	unsigned int log_cmd_{0};
	unsigned int log_msg_cmd_{0};
	unsigned int begin_cmd_{0};

	unsigned int task_init_{0};
	unsigned int task_complete_{0};
	unsigned int task_wait_{0};
	unsigned int task_abort_{0};
	unsigned int task_event_{0};
	unsigned int task_meter_{0};
   unsigned int task_label_{0};
   unsigned int task_queue_{0};

	unsigned int zombie_fob_{0};
	unsigned int zombie_fail_{0};
	unsigned int zombie_adopt_{0};
	unsigned int zombie_remove_{0};
	unsigned int zombie_get_{0};
   unsigned int zombie_block_{0};
   unsigned int zombie_kill_{0};

	unsigned int requeue_node_{0};
	unsigned int order_node_{0};
	unsigned int run_node_{0};
 	unsigned int load_defs_{0};
	unsigned int replace_{0};
	unsigned int force_{0};
	unsigned int free_dep_{0};
	unsigned int suites_{0};
	unsigned int edit_script_{0};

	unsigned int alter_cmd_{0};
   unsigned int ch_cmd_{0};
	unsigned int file_ecf_{0};
	unsigned int file_job_{0};
   unsigned int file_jobout_{0};
   unsigned int file_cmdout_{0};
	unsigned int file_manual_{0};

	unsigned int plug_{0};
	unsigned int move_{0};
   unsigned int group_cmd_{0};
   unsigned int server_load_cmd_{0};
   unsigned int stats_{0};
   unsigned int check_{0};
   unsigned int query_{0};

private:

	std::deque< std::pair<int,int> > request_vec_; // pair.first =  number of requests, pair.second = poll interval

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
		ar & status_;
		ar & locked_by_user_;
		ar & host_;
		ar & port_;
		ar & up_since_;
		ar & version_;
		ar & job_sub_interval_;
      ar & checkpt_interval_;
      ar & checkpt_save_time_alarm_;
      ar & checkpt_mode_;
		ar & no_of_suites_;
		ar & request_stats_;
		ar & ECF_HOME_;
		ar & ECF_CHECK_;
		ar & ECF_LOG_;

		ar & checkpt_;

      ar & server_version_;
      ar & restart_server_;
		ar & shutdown_server_;
		ar & halt_server_;
 		ar & reload_white_list_file_;
 		ar & reload_passwd_file_;
 		ar & ping_;
 	   ar & debug_server_on_;
 	   ar & debug_server_off_;

		ar & get_defs_;
      ar & sync_;
      ar & sync_full_;
      ar & sync_clock_;
		ar & news_;

      ar & node_job_gen_;
      ar & node_check_job_gen_only_;
		ar & node_delete_;
		ar & node_suspend_;
		ar & node_resume_;
		ar & node_kill_;
		ar & node_status_;
		ar & node_edit_history_;
	   ar & node_archive_;
	   ar & node_restore_;

		ar & log_cmd_;
		ar & log_msg_cmd_;
		ar & begin_cmd_;

		ar & task_init_;
		ar & task_complete_;
		ar & task_wait_;
		ar & task_abort_;
		ar & task_event_;
		ar & task_meter_;
		ar & task_label_;
		ar & task_queue_;

		ar & zombie_fob_;
		ar & zombie_fail_;
		ar & zombie_adopt_;
		ar & zombie_remove_;
		ar & zombie_get_;
      ar & zombie_block_;
      ar & zombie_kill_;

		ar & requeue_node_;
		ar & order_node_;
		ar & run_node_;
 		ar & load_defs_;
		ar & replace_;
		ar & force_;
		ar & free_dep_;
		ar & suites_;
		ar & edit_script_;

      ar & alter_cmd_;
      ar & ch_cmd_;
		ar & file_ecf_;
		ar & file_job_;
      ar & file_jobout_;
      ar & file_cmdout_;
		ar & file_manual_;

		ar & plug_;
		ar & move_;
		ar & group_cmd_;
      ar & server_load_cmd_;
      ar & stats_;
      ar & check_;
      ar & query_;
   }
};
#endif
