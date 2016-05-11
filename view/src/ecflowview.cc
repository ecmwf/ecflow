//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #30 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <stdlib.h>
#include <stdio.h>

#define ecflowview_cc
#include "ecflowview.h"

#include "ecflow.h"
#include "ecf_node.h"

#include "edit.h"
#include "edit_label.h"
#include "edit_limit.h"
#include "edit_meter.h"
#include "edit_repeat.h"
#include "edit_variable.h"
#include "history.h"
#include "info.h"
#include "job.h"
#include "jobcheck_panel.h"
#include "jobstatus.h"
#include "manual.h"
#include "messages.h"
#include "option_panel.h"
#include "output.h"
#include "script_panel.h"
#include "suites_panel.h"
#include "timetable_panel.h"
#include "trigger_panel.h"
#include "users.h"
#include "variables.h"
#include "why.h"
#include "zombies_panel.h"

#include "user_prefs.h"
#include "fonts_prefs.h"
#include "colors_prefs.h"
#include "servers_prefs.h"
#include "host_prefs.h"

#include "simple_node.h"
#include "date.h"
#include "event_node.h"
#include "inlimit_node.h"
#include "label_node.h"
#include "late_node.h"
#include "limit_node.h"
#include "meter_node.h"
#include "node.h"
#include "super_node.h"
#include "task_node.h"
#include "time_node.h"
#include "trigger_node.h"
#include "variable_node.h"
/*
void __cyg_profile_func_enter( void *, void * )
  __attribute__ ((no_instrument_function));

void __cyg_profile_func_enter(void *thisone, void *callsite) {
  printf("E%p\n", (int*)thisone);
}

void __cyg_profile_func_exit(void *thisone, void *callsite) {
  printf("X%p\n", (int*)thisone);
}
*/

extern int xmain(int,char**); /* from uitop.cc */
extern int wmain(int,char**);

// In label_variable.cc
static ecf_node_builder<Variable *, variable_node>  build_variable(NODE_VARIABLE);
static ecf_node_builder<const Variable *, variable_node>  build_pkvariable(NODE_VARIABLE);
// In label_node.cc
static ecf_node_builder< const Label *, label_node>     build_plabel(NODE_LABEL);
// In event_node.cc
static ecf_node_builder< const Event *, event_node>     build_pevent(NODE_EVENT);
// In meter_node.cc
static ecf_node_builder< const Meter *, meter_node>     build_cmeter(NODE_METER);
// In time_node.cc
static ecf_node_builder<const ecf::TimeAttr *, time_node>     build_ctime(NODE_TIME);
static ecf_node_builder<const ecf::TodayAttr *, time_node>     build_ctoday(NODE_TIME);
// In date_node.cc
static ecf_node_builder<const DateAttr *, date_node>     build_cdate(NODE_DATE);
static ecf_node_builder<const DayAttr *, date_node>     build_cday(NODE_DATE);
static ecf_node_builder<const ecf::CronAttr *, date_node>     build_ccron(NODE_DATE);
// In limit_node.cc
static ecf_node_builder<Limit * const, limit_node>     build_pklimit(NODE_LIMIT);
static ecf_node_builder<const InLimit *, inlimit_node> build_cinlimit(NODE_INLIMIT);
static ecf_node_builder<const ecf::LateAttr *, late_node>     build_clate(NODE_LATE);
static ecf_node_builder<ecf::LateAttr *, late_node>     build_late(NODE_LATE);

static ecf_node_builder<ExpressionWrapper*, trigger_node> build_trigger(NODE_TRIGGER);
#include "repeat.h"

static ecf_node_builder<const std::pair<std::string, std::string> *, variable_node> build_pkpvariable(NODE_VARIABLE);

static ecf_node_builder<Suite*,suite_node>build_suite(NODE_SUITE);
static ecf_node_builder<Defs*,super_node>build_topnode(NODE_SUPER);
static ecf_node_builder<Node*,family_node>build_simple(NODE_FAMILY);
static ecf_node_builder<Alias*,alias_node>build_alias(NODE_ALIAS);
static ecf_node_builder<Task*, task_node> build_task(NODE_TASK);
static ecf_node_builder<Family*,family_node>build_family(NODE_FAMILY);

static ecf_node_builder<RepeatEnumerated*,repeat_enumerated_node> pprce(NODE_REPEAT_E);
static ecf_node_builder<RepeatString*,repeat_string_node> prcs(NODE_REPEAT_S); 
static ecf_node_builder<RepeatDate*,repeat_date_node> prcd(NODE_REPEAT_D);
static ecf_node_builder<RepeatInteger*,repeat_integer_node> prci(NODE_REPEAT_I);
static ecf_node_builder<RepeatDay*,repeat_day_node> prcday(NODE_REPEAT_DAY);

static panel_maker<info> info_maker(PANEL_INFO);
static panel_maker<manual> manual_maker(PANEL_MANUAL);
static panel_maker<script_panel> script_maker(PANEL_SCRIPT);
static panel_maker<job> job_maker(PANEL_JOB);
static panel_maker<jobstatus> jobstatus_maker(PANEL_JOBSTATUS);
static panel_maker<output> output_maker(PANEL_OUTPUT);
static panel_maker<why> why_maker(PANEL_WHY);
static panel_maker<trigger_panel> trigger_panel_maker(PANEL_TRIGGER);
static panel_maker<jobcheck_panel> jobcheck_maker(PANEL_JOBCHECK);
static panel_maker<timetable_panel> timetable_maker(PANEL_TIMETABLE);
static panel_maker<variables> variables_maker(PANEL_VARIABLES);
static panel_maker<edit> edit_maker(PANEL_EDIT_TASK);
static panel_maker<edit_label> edit_label_maker(PANEL_EDIT_LABEL);
static panel_maker<edit_limit> edit_limit_maker(PANEL_EDIT_LIMIT);
static panel_maker<edit_variable> edit_variable_maker(PANEL_EDIT_VARIABLE);
static panel_maker<edit_meter> edit_meter_maker(PANEL_EDIT_METER);
static panel_maker<edit_repeat> edit_repeat_maker(PANEL_EDIT_REPEAT);
static panel_maker<history> history_maker(PANEL_HISTORY);
static panel_maker<messages> messages_maker(PANEL_MESSAGES);
static panel_maker<suites_panel> suites_maker(PANEL_SUITES);
static panel_maker<users> users_maker(PANEL_USERS);
static panel_maker<zombies_panel> zombies_maker(PANEL_ZOMBIES);
static panel_maker<option_panel> maker(PANEL_ECF_OPTIONS);

static user_prefs    user_hp;
static colors_prefs  colors_hp;
static fonts_prefs   fonts_hp;
static host_prefs    hosts_hp;

#ifdef BRIDGE
static repeat_node_maker repeat_node_maker_instance;
late_node::late_node(host& h,sms_node* n, char b) : node(h,n,b), label_(0) {}
#endif

int main(int argc,char** argv)
{
  return getenv("ECFLOW_HTTP_PORT") ? wmain(argc,argv) : 
    xmain(argc,argv);
  ecf_nick_write();
}

