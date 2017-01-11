#ifndef STATS_HPP_
#define STATS_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #31 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <boost/serialization/serialization.hpp>
#include <deque>
#include <iostream>
#include <sstream>
#include "CheckPt.hpp"

/// This class is used to store all statistical data about all the
/// commands processed by the server. Uses default copy constructor
struct Stats {

	Stats();
  void show(std::ostream& os = std::cout) const;

   void update() { request_count_++; }
   void update_stats(int poll_interval);
   void update_for_serialisation();
   void reset();

	int status_;    // 0 HALTED, 1 SHUTDOWN, 2 RUNNING
	std::string locked_by_user_;
	std::string host_;
	std::string port_;
	std::string up_since_;
	std::string version_;
	std::string request_stats_;
   std::string ECF_HOME_;
   std::string ECF_CHECK_;
   std::string ECF_LOG_;

	int request_count_;
	int job_sub_interval_;
   int checkpt_interval_;
   int checkpt_save_time_alarm_;
   ecf::CheckPt::Mode checkpt_mode_;
	int no_of_suites_;

	unsigned int checkpt_;
	unsigned int restore_defs_from_checkpt_;

   unsigned int server_version_;
   unsigned int restart_server_;
	unsigned int shutdown_server_;
	unsigned int halt_server_;
 	unsigned int reload_white_list_file_;
#ifdef ECF_SECURE_USER
 	unsigned int reload_passwd_file_;
#endif
   unsigned int ping_;
   unsigned int debug_server_on_;
   unsigned int debug_server_off_;
	unsigned int get_defs_;
	unsigned int sync_;
	unsigned int news_;

   unsigned int node_job_gen_;
   unsigned int node_check_job_gen_only_;
	unsigned int node_delete_;
	unsigned int node_suspend_;
	unsigned int node_resume_;
	unsigned int node_kill_;
   unsigned int node_status_;
   unsigned int node_edit_history_;

	unsigned int log_cmd_;
	unsigned int log_msg_cmd_;
	unsigned int begin_cmd_;

	unsigned int task_init_;
	unsigned int task_complete_;
	unsigned int task_wait_;
	unsigned int task_abort_;
	unsigned int task_event_;
	unsigned int task_meter_;
	unsigned int task_label_;

	unsigned int zombie_fob_;
	unsigned int zombie_fail_;
	unsigned int zombie_adopt_;
	unsigned int zombie_remove_;
	unsigned int zombie_get_;
   unsigned int zombie_block_;
   unsigned int zombie_kill_;

	unsigned int requeue_node_;
	unsigned int order_node_;
	unsigned int run_node_;
 	unsigned int load_defs_;
	unsigned int replace_;
	unsigned int force_;
	unsigned int free_dep_;
	unsigned int suites_;
	unsigned int edit_script_;

	unsigned int alter_cmd_;
   unsigned int ch_cmd_;
	unsigned int file_ecf_;
	unsigned int file_job_;
   unsigned int file_jobout_;
   unsigned int file_cmdout_;
	unsigned int file_manual_;

	unsigned int plug_;
	unsigned int move_;
   unsigned int group_cmd_;
   unsigned int server_load_cmd_;
   unsigned int stats_;
   unsigned int check_;

private:

	std::deque< std::pair<int,int> > request_vec_; // pair.first =  number of requests, pair.second = poll interval
	friend class boost::serialization::access;
	template<class Archive>
	void serialize( Archive & ar, const unsigned int /*version*/ ) {

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
#ifdef ECF_SECURE_USER
 		ar & reload_passwd_file_;
#endif
 		ar & ping_;
 	   ar & debug_server_on_;
 	   ar & debug_server_off_;

		ar & get_defs_;
		ar & sync_;
		ar & news_;

      ar & node_job_gen_;
      ar & node_check_job_gen_only_;
		ar & node_delete_;
		ar & node_suspend_;
		ar & node_resume_;
		ar & node_kill_;
		ar & node_status_;
		ar & node_edit_history_;

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
   }
};
#endif
