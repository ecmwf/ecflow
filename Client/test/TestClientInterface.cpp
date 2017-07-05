//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #75 $ 
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
#include <fstream>
#include <iostream>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/foreach.hpp>

#include "ClientInvoker.hpp"
#include "ClientToServerCmd.hpp"
#include "ClientEnvironment.hpp"
#include "NState.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "Suite.hpp"
#include "Child.hpp"
#include "File.hpp"
#include "Flag.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// **************************************************************************************
// test the interface, this will create the cmd without actually submitting to the server
// This will test argument parsing.
// **************************************************************************************
BOOST_AUTO_TEST_CASE( test_client_interface )
{
   std::cout << "Client:: ...test_client_interface" << endl;

   ClientInvoker theClient ;
   theClient.testInterface(); // stops submission to server
   std::vector<std::string> paths; paths.push_back("/s1/t1"); paths.push_back("/s2/t2");

   BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.terminateServer() == 0,CtsApi::terminateServer() << " should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.server_version() == 0,CtsApi::server_version() << " should return 0\n" << theClient.errorMsg());

   {
      defs_ptr dummy_defs = Defs::create();
      BOOST_REQUIRE_MESSAGE( theClient.sync(dummy_defs) == 0, std::string(CtsApi::syncArg()) << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.news(dummy_defs) == 0, std::string(CtsApi::newsArg()) << " should return 0\n" << theClient.errorMsg());
   }

   BOOST_REQUIRE_MESSAGE( theClient.server_load("/path/to_log/file") == 0,CtsApi::server_load("/path/to_log/file") << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.server_load() == 0,CtsApi::server_load("") << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.pingServer() == 0,CtsApi::pingServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.debug_server_on() == 0,CtsApi::debug_server_on() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.debug_server_off() == 0,CtsApi::debug_server_off() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.stats() == 0,CtsApi::stats() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.stats_server() == 0,CtsApi::stats_server() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.stats_reset() == 0,CtsApi::stats_reset() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.suites() == 0,CtsApi::suites() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.getDefs() == 0,CtsApi::get() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.begin("/suite") == 0,CtsApi::begin("/suite") << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.begin("/suite",true) == 0,CtsApi::begin("/suite",true) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.begin("/suite",false) == 0,CtsApi::begin("/suite",false) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.begin_all_suites() == 0,CtsApi::begin() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("event","/suite","event") == 0, CtsApi::queryArg() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("meter","/suite","meter") == 0, CtsApi::queryArg() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("variable","/suite","var") == 0, CtsApi::queryArg() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("trigger","/suite","1 == 1") == 0, CtsApi::queryArg() << " should return 0\n" << theClient.errorMsg());


   Zombie z(Child::USER,ecf::Child::INIT,ZombieAttr::get_default_attr(Child::USER),"/path/to/task","DUMMY_JOBS_PASSWORD", "DUMMY_PROCESS_OR_REMOTE_ID",1);
   BOOST_REQUIRE_MESSAGE( theClient.zombieGet() == 0,CtsApi::zombieGet() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieFob(z) == 0,    " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieFail(z) == 0,   " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieAdopt(z) == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieBlock(z) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieRemove(z) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieKill(z) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieFobCli("/path/to/task") == 0,    " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieFailCli("/path/to/task") == 0,   " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieAdoptCli("/path/to/task") == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieBlockCli("/path/to/task") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieRemoveCli("/path/to/task") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieKillCli("/path/to/task") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieFobCliPaths(paths) == 0,    " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieFailCliPaths(paths) == 0,   " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieAdoptCliPaths(paths) == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieBlockCliPaths(paths) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieRemoveCliPaths(paths) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.zombieKillCliPaths(paths) == 0, " should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.job_gen("") == 0,CtsApi::job_gen("") << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.job_gen("/s") == 0,CtsApi::job_gen("/s") << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.check("") == 0,CtsApi::to_string(CtsApi::check("")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.check("/") == 0,CtsApi::to_string(CtsApi::check("/")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.check("_all_") == 0,CtsApi::to_string(CtsApi::check("_all_")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.check("/s") == 0,CtsApi::to_string(CtsApi::check("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.check(paths) == 0,CtsApi::to_string(CtsApi::check(paths)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.kill("/s") == 0,CtsApi::to_string(CtsApi::kill("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.kill(paths) == 0,CtsApi::to_string(CtsApi::kill(paths)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.status("/s") == 0,CtsApi::to_string(CtsApi::status("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.status(paths) == 0,CtsApi::to_string(CtsApi::status(paths)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.suspend("/s") == 0,CtsApi::to_string(CtsApi::suspend("/s"))<< " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.suspend(paths) == 0,CtsApi::to_string(CtsApi::suspend(paths))<< " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.resume("/s") == 0,CtsApi::to_string(CtsApi::resume("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.resume(paths) == 0,CtsApi::to_string(CtsApi::resume(paths)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.edit_history("/s") == 0,CtsApi::to_string(CtsApi::edit_history("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.archive("/s") == 0,CtsApi::to_string(CtsApi::archive("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.archive(paths) == 0,CtsApi::to_string(CtsApi::archive(paths)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.archive("/s",true) == 0,CtsApi::to_string(CtsApi::archive("/s",true)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.archive(paths,true) == 0,CtsApi::to_string(CtsApi::archive(paths,true)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.restore("/s") == 0,CtsApi::to_string(CtsApi::restore("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.restore(paths) == 0,CtsApi::to_string(CtsApi::restore(paths)) << " should return 0\n" << theClient.errorMsg());

   // empty string should be same as delete all
   BOOST_REQUIRE_MESSAGE( theClient.delete_node("") == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_node("_all_") == 0,CtsApi::to_string(CtsApi::delete_node("_all")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_node("_all_",true) == 0,CtsApi::to_string(CtsApi::delete_node("_all",true)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_node("_all_",false) == 0,CtsApi::to_string(CtsApi::delete_node("_all",false)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_node("/s") == 0,CtsApi::to_string(CtsApi::delete_node("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_node("/s",true) == 0,CtsApi::to_string(CtsApi::delete_node("/s",true)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_node("/s",false) == 0,CtsApi::to_string(CtsApi::delete_node("/s",false)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_nodes(paths,false) == 0,CtsApi::to_string(CtsApi::delete_node(paths,false)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_nodes(paths,true) == 0,CtsApi::to_string(CtsApi::delete_node(paths,false)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.requeue("/s","") == 0,      CtsApi::to_string(CtsApi::requeue("/s","")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.requeue("/s","force") == 0, CtsApi::to_string(CtsApi::requeue("/s","force")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.requeue("/s","abort") == 0, CtsApi::to_string(CtsApi::requeue("/s","abort")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.requeue(paths,"") == 0,     CtsApi::to_string(CtsApi::requeue(paths,"")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.requeue(paths,"force") == 0,CtsApi::to_string(CtsApi::requeue(paths,"force")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.requeue(paths,"abort") == 0,CtsApi::to_string(CtsApi::requeue(paths,"abort")) << " should return 0\n" << theClient.errorMsg());


   BOOST_REQUIRE_MESSAGE( theClient.run("/s") == 0,CtsApi::to_string(CtsApi::run("/s")) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.run("/s",true) == 0,CtsApi::to_string(CtsApi::run("/s",true)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.run(paths) == 0,CtsApi::to_string(CtsApi::run(paths)) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.run(paths,true) == 0,CtsApi::to_string(CtsApi::run(paths,true)) << " should return 0\n" << theClient.errorMsg());


   std::string path = File::test_data("Client/test/data/lifecycle.txt","Client");
   BOOST_REQUIRE_MESSAGE( theClient.loadDefs(path) == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.loadDefs(path,true/*force*/) == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.loadDefs(path,true/*force*/,true/*check_only*/) == 0,  "should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.replace("/suite1",path) == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.replace("/suite1",path,true) == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.replace("/suite1",path,true, true) == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.replace("/suite1",path,false, true) == 0,  " should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.order("/s","top") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.order("/s","bottom") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.order("/s","alpha") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.order("/s","order") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.order("/s","up") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.order("/s","down") == 0, " should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.checkPtDefs() == 0,CtsApi::checkPtDefs() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.checkPtDefs(ecf::CheckPt::NEVER) == 0," should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.checkPtDefs(ecf::CheckPt::ON_TIME) == 0," should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.checkPtDefs(ecf::CheckPt::ON_TIME,180) == 0," should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.checkPtDefs(ecf::CheckPt::ALWAYS) == 0," should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.checkPtDefs(ecf::CheckPt::UNDEFINED) == 0," should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.checkPtDefs(ecf::CheckPt::UNDEFINED,0,35) == 0," should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.restoreDefsFromCheckPt() == 0,CtsApi::restoreDefsFromCheckPt() << " should return 0\n" << theClient.errorMsg());

   std::vector<std::string> event_paths; event_paths.push_back("/s1:e"); event_paths.push_back("/s2/f1/t1:e");
   BOOST_REQUIRE_MESSAGE( theClient.force(event_paths[0],"set") == 0,   " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.force(event_paths[1],"clear") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.force(event_paths,"set") == 0,  " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.force(event_paths,"clear") == 0," should return 0\n" << theClient.errorMsg());
   std::vector<std::string> validStates = NState::allStates(); // HPUX barfs if use NState::allStates() directly
   BOOST_FOREACH(const string& state, validStates) {
      BOOST_REQUIRE_MESSAGE( theClient.force("/s",state,true,true) == 0, "force " << state << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.force(paths,state,true,true) == 0, "force " << state << " should return 0\n" << theClient.errorMsg());
   }

   BOOST_REQUIRE_MESSAGE( theClient.freeDep("/s") == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.freeDep("/s",true,true,true,true) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.freeDep("/s",true) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.freeDep("/s",false,true) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.freeDep("/s",false,false,true) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.freeDep("/s",false,false,false,true) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.freeDep(paths) == 0, " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.freeDep(paths,true,true,true,true) == 0, " should return 0\n" << theClient.errorMsg());

   std::vector<CFileCmd::File_t> fileTypesVec = CFileCmd::fileTypesVec();
   for(size_t i = 0; i < fileTypesVec.size(); i++) {
      BOOST_REQUIRE_MESSAGE( theClient.file("/s",CFileCmd::toString(fileTypesVec[i]),string("100")) == 0, " should return 0\n" << theClient.errorMsg());
   }

   BOOST_REQUIRE_MESSAGE( theClient.plug("/source","/dest") == 0, " should return 0\n" << theClient.errorMsg());

   {
      BOOST_REQUIRE_MESSAGE( theClient.getLog(10) == 0," should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.clearLog() == 0, CtsApi::clearLog() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.flushLog() == 0, CtsApi::flushLog() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.get_log_path() == 0, CtsApi::get_log_path() << " should return 0\n" << theClient.errorMsg());

      std::string new_log_path = File::test_data("Client/test/data/new_log.log","Client");
      BOOST_REQUIRE_MESSAGE( theClient.new_log(new_log_path) == 0, " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.new_log("") == 0, " should return 0\n" << theClient.errorMsg());
   }

   BOOST_REQUIRE_MESSAGE( theClient.reloadwsfile() == 0,CtsApi::reloadwsfile() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.forceDependencyEval() == 0, CtsApi::forceDependencyEval() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.group("shutdown yes;halt yes;restart") == 0,"--group should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.group("shutdown=yes;halt=yes;restart") == 0,"--group should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.group("get ; show") == 0,"--group should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.group("get ; show state") == 0,"--group should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.group("get ; show defs") == 0,"--group should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.group("get ; show migrate") == 0,"--group should return 0\n" << theClient.errorMsg());


   {
      std::string path_to_script = "dummy_ecf_file" + File::ECF_EXTN();
      std::string contents = "%comment\n"
               "VAR =  fred\n"
               "%end\n"
               "# rest of the ecf file\n";
      std::string error_msg;
      BOOST_CHECK_MESSAGE(File::create(path_to_script,contents,error_msg),error_msg);

      std::vector<std::string> file_contents;
      NameValueVec used_variables;
      BOOST_REQUIRE_MESSAGE( theClient.edit_script_edit("/s") == 0,"--edit_script_edit should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script_preprocess("/s") == 0,"--edit_script_preprocess should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script_preprocess("/s",file_contents) == 0,"--edit_script_preprocess should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script_submit("/s",used_variables) == 0,"--edit_script_submit should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script_submit("/s",used_variables,file_contents) == 0,"--edit_script_submit should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script_submit("/s",used_variables,file_contents,true/*alias*/) == 0,"--edit_script_submit should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script_submit("/s",used_variables,file_contents,true/*alias*/,true/*run*/) == 0,"--edit_script_submit should return 0\n" << theClient.errorMsg());

      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","edit") == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","pre_process") == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","submit",path_to_script) == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","submit_file",path_to_script) == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","submit_file",path_to_script,true) == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","submit_file",path_to_script,false) == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","submit_file",path_to_script,false,true) == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","submit_file",path_to_script,false,false) == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.edit_script("/s","pre_process_file",path_to_script) == 0,"--edit_script should return 0\n" << theClient.errorMsg());
      fs::remove(path_to_script);
   }

   {
      std::vector<std::string> suites; suites.push_back("a"); suites.push_back("b");
      BOOST_REQUIRE_MESSAGE( theClient.ch_register(true,suites) == 0,"--ch_register \n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_register(false,suites) == 0,"--ch_register \n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_suites() == 0,"--ch_suites should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_drop(1) == 0,"--ch_drop should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_drop_user("user") == 0,"--ch_drop_user should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_add(1,suites) == 0,"--ch_add \n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_remove(1,suites) == 0,"--ch_remove \n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_auto_add(1,true) == 0,"--ch_auto_add \n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.ch_auto_add(1,false) == 0,"--ch_auto_add \n" << theClient.errorMsg());
      // need test interface that allows client handle to set on ClinetInvoker
//      BOOST_REQUIRE_MESSAGE( theClient.ch1_drop() == 0,"--ch1_drop \n" << theClient.errorMsg());
//      BOOST_REQUIRE_MESSAGE( theClient.ch1_add(suites) == 0,"--ch1_add \n" << theClient.errorMsg());
//      BOOST_REQUIRE_MESSAGE( theClient.ch1_remove(suites) == 0,"--ch1_remove \n" << theClient.errorMsg());
//      BOOST_REQUIRE_MESSAGE( theClient.ch1_auto_add(true) == 0,"--ch1_auto_add \n" << theClient.errorMsg());
//      BOOST_REQUIRE_MESSAGE( theClient.ch1_auto_add(false) == 0,"--ch1_auto_add \n" << theClient.errorMsg());
   }

   {
      defs_ptr theDefs = Defs::create();
      theDefs->addSuite( Suite::create("s1") );
      BOOST_REQUIRE_MESSAGE( theClient.load( theDefs ) == 0,"-- load should return 0\n" << theClient.errorMsg());
   }

   /// test alter
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:fob::10") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:fob::") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","user:fob::10") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","path:fob::10") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","path:fob::") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:fob:init,event,meter,label,wait,complete,abort:10") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:fob:init,event,meter,label,wait,complete:10") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:fail:init,event,meter,label,wait,complete:10000") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:adopt:init,event,meter,label,wait,complete:23") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:remove:init,event,meter,label,wait,complete:0") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:block:init,event,meter,label,wait,complete:") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","zombie","ecf:kill:init,event,meter,label,wait,complete:103333333") == 0,"--alter should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","+12:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","+10:00 20:00 00:30") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","12:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","10:00 20:00 00:30") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","12.12.2009") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","*.12.2009") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","*.*.2009") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","*.*.*") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","variable","name","value") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","variable","name","/value/with/path") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/",  "add","variable","name","value") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/",  "add","variable","name","/value/with/path") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -s +00:15 -a 20:00 -c +02:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -a 20:00 -c +02:00 -s +00:15") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -c +02:00 -a 20:00  -s +00:15") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -s 00:02 -c +00:05") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -s 00:01 -a 14:30 -c +00:01") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-s +00:15 -a 20:00 -c +02:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-a 20:00 -c +02:00 -s +00:15") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-c +02:00 -a 20:00  -s +00:15") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-s 00:02 -c +00:05") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-s 00:01 -a 14:30 -c +00:01") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","limit","limit_name","10") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","inlimit","limit_name") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","inlimit","limit_name","10") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","inlimit","/path/to/limit:limit_name2") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","inlimit","/path/to/limit:limit_name2","10") == 0,"--alter should return 0\n" << theClient.errorMsg());


   std::vector<std::string> validDays = DayAttr::allDays(); // HPUX barfs if use DayAttr::allDays() directly in BOOST_FOREACH
   BOOST_FOREACH(const string& day, validDays) {
      BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","day",day) == 0,"--alter should return 0\n" << theClient.errorMsg());
   }
   BOOST_REQUIRE_MESSAGE( theClient.alter("/",  "delete","variable","varName") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","variable","varName") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","variable","/var/with/path") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","variable") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","+12:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","+10:00 20:00 00:30") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today","12:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today","10:00 20:00 00:30") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","12.12.2009") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","*.12.2009") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","*.*.2009") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","*.*.*") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","day","sunday") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","day") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","23:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","10:00 20:00 01:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 0,1 10:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-d 10,11,12 12:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 0 -m 5,6,7,8 10:00 20:00 01:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","event","name") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","event","1") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","event") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","meter","name") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","meter") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","trigger") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","complete") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","repeat") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","limit","name") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","limit_path","limit_name","path") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","limit_path","limit_name","/path/with/slash") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","inlimit","name") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","zombie","user") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","zombie","ecf") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","zombie","path") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","late") == 0,"--alter should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","variable","name","newValue") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","variable","name","/new/value/with/path") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","hybrid") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","real") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_date","12.6.2013") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_gain","20") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_sync") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","22","set") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","33","clear") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","4") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","name","set") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","name","clear") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","name") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","meter","name","20") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","meter","name","-1") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","label","name","newValue") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","label","name","/value/with/path/ECFLOW-480") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","label","name","") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","trigger","(t:step + 20) ge (t:step1 - 20)") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","trigger","/suite/fred == complete") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","trigger","/suite/fred:event") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","complete","not ( a == complete )") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","complete","/trigger/with/leading/slash == aborted") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","complete","/trigger/with/leading/slash:event") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","repeat","1") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","repeat","blue") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","limit_max","limit_name","12") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","limit_value","limit_name","12") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","late -s +00:15 -a 20:00 -c +02:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","late -s 00:02 -c +00:05") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","late -s 00:01 -a 14:30 -c +00:01") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-s +00:15 -a 20:00 -c +02:00") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-a 20:00 -c +02:00 -s +00:15") == 0,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-c +02:00 -a 20:00  -s +00:15") == 0,"--alter should return 0\n" << theClient.errorMsg());


   std::vector<std::string> dstates = DState::allStates();
   for(size_t i = 0; i < dstates.size(); i++) {
      BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","defstatus",dstates[i]) == 0,"--alter should return 0\n" << theClient.errorMsg());
   }

   std::vector<std::string> flag_types;
   Flag::valid_flag_type(flag_types);
   for(size_t i = 0; i <  flag_types.size(); i++) {
      BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","set_flag",flag_types[i]) == 0,"--alter should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","clear_flag",flag_types[i]) == 0,"--alter should return 0\n" << theClient.errorMsg());
   }

   // sort
   std::vector<std::string> sortable_attributes = Attr::all_attrs();
   BOOST_REQUIRE_MESSAGE(!sortable_attributes.empty(),"Expected to find attributes");
   for(size_t i = 0; i <  sortable_attributes.size(); i++) {
      BOOST_REQUIRE_MESSAGE( theClient.alter_sort("/s1",sortable_attributes[i],true) == 0,"--alter should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.alter_sort("/s1",sortable_attributes[i],false) == 0,"--alter should return 0\n" << theClient.errorMsg());
   }
}

BOOST_AUTO_TEST_CASE( test_client_interface_for_fail )
{
   std::cout << "Client:: ...test_client_interface_for_fail" << endl;

   ClientInvoker theClient ;
   theClient.testInterface(); // stops submission to server
   theClient.set_throw_on_error(false);

   std::vector<std::string> paths; paths.push_back("/s1"); paths.push_back("/s2");

   BOOST_REQUIRE_MESSAGE( theClient.check("fred") == 1,CtsApi::to_string(CtsApi::check("fred")) << " should return 1\n" << theClient.errorMsg());

   /// test alter
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","012:00") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","1200") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","fred") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","+10:0020:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","+10:00 2000 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","+1000 20:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","+10:00 20:00 0030") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","time","any old rubbish") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","+10:0020:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","+10:00 2000 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","+1000 20:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","+10:00 20:00 0030") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","any old rubbish") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","today","") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","0.12.2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","12.0.2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","12.12.g") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","*..2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","12..2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","12.12.") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","fred") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","date","any old rubbish") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","day","frenday") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","day","") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","day","opps ") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","variable","na me","value") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/",  "add","variable","na me","value") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/",  "add","variable","","value") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -s") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -a") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -c") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -s +00:15 -a") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","late -s +00:15 -a 20:00 -c") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-s") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-a") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-c") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-s +00:15 -a") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-s +00:15 -a 20:00 -c") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","add","late","-c +02:000 -a 20:00  -s +00:15") == 1,"--alter should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/",  "delete","vari able","varName") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","v ariable","varName") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","012:00") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","1200") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","fred") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","+10:0020:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","+10:00 2000 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","+1000 20:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","+10:00 20:00 0030") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","time","any old rubbish") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today","+10:0020:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today","+10:00 2000 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today","+1000 20:00 00:30") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today","+10:00 20:00 0030") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","today","any old rubbish") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","0.12.2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","12.0.2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","12.12.g") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","*..2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","12..2009") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","12.12.") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","fred") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","date","any old rubbish") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","day","su nday") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","day","fryday") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","any old rubbish") == 1,"Expected fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","2300") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","1000 20:00 01:00") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 0,1 1000'") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 1000") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-d 10,11,12 1200") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-d 10,11,12") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-d 1200") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 0 -m 5,6,7,8 10:00 20:00 01:000") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 0 -m 5,6,7,8 10:00 20:00 0100") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w -m 20:00 01:00") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 0 -m 5,6,7,8 1000 20:00 01:00") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 20 -m 5,6,7,8 10:00 20:00 01:00") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","cron","-w 0 -m 5,6,7,8,24 10:00 20:00 01:00") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","event","name df") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","event","22  56  ") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","meter","na me") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","limit"," name") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","delete","inlimit","na me") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","vari able","name","newValue") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","","name","/new/value/with/path") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","hyb rid") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","hybrid ") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","ybrid") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","re al") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_type","rreal") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_date","*.6.2013") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_date","12.0.2013") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_date","any old rubbish") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_date","") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_gain","string") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_gain","") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","clock_gain","fred") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","name","fr ed") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","event","na me") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","meter","name","") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","meter","na me","20") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","meter","name","-") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","label","na me","newValue") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","trigger","t:step + 20) ge (t:step1 - 20)") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","complete","not  a == complete )") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","limit_max","limit_  name","12") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","limit_value","limit_  name","12") == 1,"--alter expected to fail\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","defstatus","complete-34") == 1,"--alter expected to fail\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","late") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","late -s") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","late -a") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","late -c") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-s +00:15 -a") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-s +00:15 -a 20:00 -c") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-s +00:15 -a") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-s +00:15 -a 20:00 -c") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-s +00:152") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-c +00:152") == 1,"--alter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.alter("/s1","change","late","-a +00:152") == 1,"--alter should return 0\n" << theClient.errorMsg());

   BOOST_REQUIRE_MESSAGE( theClient.query("event","/suite","") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("meter","/suite","") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("variable","/suite","") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("trigger","/suite","") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("event","","event") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("meter","","meter") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("variable","","var") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("trigger","","1 == 1") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("xxxx","/suite","1 == 1") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.query("","/suite","1 == 1") == 1, std::string(CtsApi::queryArg()) << " should return 0\n" << theClient.errorMsg());
}


BOOST_AUTO_TEST_CASE( test_client_task_interface )
{
   std::cout << "Client:: ...test_client_task_interface" << endl;

   ClientInvoker theClient ;
   theClient.testInterface(); // stops submission to server
   theClient.taskPath("/a/made/up/path");
   theClient.set_jobs_password( Submittable::DUMMY_JOBS_PASSWORD() );

   BOOST_REQUIRE_MESSAGE( theClient.initTask(Submittable::DUMMY_PROCESS_OR_REMOTE_ID()) == 0,"--init should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.abortTask("reason for abort") == 0,"--abort should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.eventTask("event_name") == 0,"--event should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.meterTask("meter_name","20") == 0,"--meter should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.waitTask("a == complete") == 0,"--wait should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.queueTask("queue") == 0,"--queue should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.queueTask("queue","/path/to/node/with/queue") == 0,"--queue should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.completeTask() == 0,"--complete should return 0\n" << theClient.errorMsg());
   std::vector<std::string> labels; labels.push_back("test_client_task_interface");
   BOOST_REQUIRE_MESSAGE( theClient.labelTask("label_name",labels) == 0,"--label should return 0\n" << theClient.errorMsg());
}

BOOST_AUTO_TEST_SUITE_END()

