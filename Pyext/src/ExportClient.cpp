/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #85 $ 
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
#include <boost/python.hpp>
#include <boost/noncopyable.hpp>

#include "ClientInvoker.hpp"
#include "Defs.hpp"
#include "ClientDoc.hpp"
#include "WhyCmd.hpp"
#include "UrlCmd.hpp"
#include "NState.hpp"
#include "Version.hpp"
#include "BoostPythonUtil.hpp"

using namespace boost::python;
using namespace std;
namespace bp = boost::python;

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

void set_host_port(ClientInvoker* self, const std::string& host,int port) {self->set_host_port(host,boost::lexical_cast<std::string>(port));}
void set_host_port_1(ClientInvoker* self, const std::string& host_port){
   // assume format <host>:<port>
   size_t colonPos = host_port.find_first_of(':');
   if (colonPos == string::npos)  throw std::runtime_error("set_host_port: expected <host>:<port> : no ':' found in " + host_port);
   std::string host = host_port.substr(0,colonPos);
   std::string port = host_port.substr(colonPos+1);
   self->set_host_port(host,port);
}


std::string version(ClientInvoker* self) { return ecf::Version::raw();}
std::string server_version(ClientInvoker* self) { self->server_version(); return self->get_string();}

const std::string& query(ClientInvoker* self,
                         const std::string& query_type,
                         const std::string& path_to_attribute,
                         const std::string& attribute) {
   self->query(query_type,path_to_attribute,attribute); return self->get_string();}

const std::string& get_log(ClientInvoker* self) { self->getLog(); return self->get_string();}

const std::string& get_file(ClientInvoker* self,
                            const std::string& absNodePath,
                            const std::string& file_type = "script",
                            const std::string& max_lines = "10000")
{ self->file(absNodePath,file_type,max_lines); return self->get_string(); }

const std::string& get_file_1(ClientInvoker* self,
                            const std::string& absNodePath,
                            const std::string& file_type = "script" )
{ self->file(absNodePath,file_type,"10000"); return self->get_string(); }


/// Set the CLI to enable output to standard out
class CliSetter {
public:
   CliSetter(ClientInvoker* self) : _self(self) {  self->set_cli(true); }
   ~CliSetter()                                 { _self->set_cli(false);}
private:
   ClientInvoker* _self;
};
void stats(ClientInvoker* self) { CliSetter setter(self); self->stats(); }
void stats_reset(ClientInvoker* self) { self->stats_reset(); }
boost::python::list suites(ClientInvoker* self) {
   self->suites();
   const std::vector<std::string> the_suites = self->server_reply().get_string_vec();
   boost::python::list list;
   size_t the_size = the_suites.size();
   for(size_t i = 0; i < the_size; i++)  list.append( the_suites[i] );
   return list;
}

bool news_local(ClientInvoker* self) { self->news_local(); return self->get_news(); }

void free_trigger_dep(ClientInvoker* self, const std::string& path) { self->freeDep(path,true /*trigger*/,false/*all*/,false/*date*/,false/*time*/); }
void free_date_dep(ClientInvoker* self, const std::string& path)    { self->freeDep(path,false/*trigger*/,false/*all*/,true /*date*/,false/*time*/); }
void free_time_dep(ClientInvoker* self, const std::string& path)    { self->freeDep(path,false/*trigger*/,false/*all*/,false/*date*/,true /*time*/); }
void free_all_dep(ClientInvoker* self, const std::string& path)     { self->freeDep(path,false/*trigger*/,true /*all*/,false/*date*/,false/*time*/); }
void free_trigger_dep1(ClientInvoker* self,const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->freeDep(paths,true /*trigger*/,false/*all*/,false/*date*/,false/*time*/); }
void free_date_dep1(ClientInvoker* self,   const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->freeDep(paths,false/*trigger*/,false/*all*/,true /*date*/,false/*time*/); }
void free_time_dep1(ClientInvoker* self,   const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->freeDep(paths,false/*trigger*/,false/*all*/,false/*date*/,true /*time*/); }
void free_all_dep1(ClientInvoker* self,    const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->freeDep(paths,false/*trigger*/,true /*all*/,false/*date*/,false/*time*/); }

void force_state(ClientInvoker* self, const std::string& path, NState::State state)         { self->force(path,NState::toString(state),false);}
void force_states(ClientInvoker* self, const boost::python::list& list, NState::State state){ std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->force(paths,NState::toString(state),false);}
void force_state_recursive(ClientInvoker* self, const std::string& path, NState::State state) { self->force(path,NState::toString(state),true);}
void force_states_recursive(ClientInvoker* self,const boost::python::list& list,NState::State state){ std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->force(paths,NState::toString(state),true);}
void force_event(ClientInvoker* self, const std::string& path, const std::string& set_or_clear)        { self->force(path,set_or_clear);}
void force_events(ClientInvoker* self,const boost::python::list& list,const std::string& set_or_clear) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->force(paths,set_or_clear);}

void run(ClientInvoker* self, const std::string& path,   bool force)       { self->run(path,force);}
void runs(ClientInvoker* self,const boost::python::list& list, bool force) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths); self->run(paths,force);}
void requeue(ClientInvoker* self,std::string path, const std::string& option) { self->requeue(path,option);}
void requeues(ClientInvoker* self,const boost::python::list& list, const std::string& option) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->requeue(paths,option);}
void suspend(ClientInvoker* self, const std::string& path)               { self->suspend(path);}
void suspends(ClientInvoker* self,const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->suspend(paths);}
void resume(ClientInvoker* self, const std::string& path)          { self->resume(path);}
void resumes(ClientInvoker* self,const boost::python::list& list)  { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->resume(paths);}
void archive(ClientInvoker* self, const std::string& path)         { self->archive(path);}
void archives(ClientInvoker* self,const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->archive(paths);}
void restore(ClientInvoker* self, const std::string& path)         { self->restore(path);}
void restores(ClientInvoker* self,const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->restore(paths);}
void the_status(ClientInvoker* self, const std::string& path)      { self->status(path);}
void statuss(ClientInvoker* self,const boost::python::list& list)  { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->status(paths);}
void do_kill(ClientInvoker* self, const std::string& path)         { self->kill(path);}
void do_kills(ClientInvoker* self,const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->kill(paths);}
const std::string& check(ClientInvoker* self, const std::string& node_path)     { self->check(node_path); return self->get_string(); }
const std::string& checks(ClientInvoker* self, const boost::python::list& list) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->check(paths); return self->get_string(); }

void delete_node(ClientInvoker* self,const boost::python::list& list, bool force) { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->delete_nodes(paths,force);}

void ch_suites(ClientInvoker* self){ CliSetter cli(self);self->ch_suites();}
void ch_register(ClientInvoker* self,bool auto_add_new_suites,const boost::python::list& list)
{std::vector<std::string> suites;BoostPythonUtil::list_to_str_vec(list,suites); self->ch_register(auto_add_new_suites,suites);}

void ch_add(ClientInvoker* self,int client_handle,const boost::python::list& list)
{std::vector<std::string> suites;BoostPythonUtil::list_to_str_vec(list,suites); self->ch_add(client_handle,suites);}
void ch1_add(ClientInvoker* self,const boost::python::list& list)
{std::vector<std::string> suites;BoostPythonUtil::list_to_str_vec(list,suites); self->ch1_add(suites);}

void ch_remove(ClientInvoker* self,int client_handle,const boost::python::list& list)
{std::vector<std::string> suites;BoostPythonUtil::list_to_str_vec(list,suites); self->ch_remove(client_handle,suites);}
void ch1_remove(ClientInvoker* self,const boost::python::list& list)
{std::vector<std::string> suites;BoostPythonUtil::list_to_str_vec(list,suites); self->ch1_remove(suites);}

/// Need to provide override since the boolean argument is optional.
/// This saves on client python code, on having to specify the optional arg
void replace_1(ClientInvoker* self,const std::string& absNodePath, defs_ptr client_defs) { self->replace_1(absNodePath,client_defs);}
void replace_2(ClientInvoker* self,const std::string& absNodePath, const std::string& path_to_client_defs) { self->replace(absNodePath,path_to_client_defs);}

void order(ClientInvoker* self,const std::string& absNodePath,const std::string& the_order){ self->order(absNodePath,the_order);}

void alters(ClientInvoker* self,
          const boost::python::list& list,
          const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
          const std::string& attrType,
          const std::string& name = "",
          const std::string& value = "") { std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->check(paths);self->alter(paths,alterType,attrType,name,value); }
void alter(ClientInvoker* self,
          const std::string& path,
          const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
          const std::string& attrType,
          const std::string& name = "",
          const std::string& value = "") { self->alter(path,alterType,attrType,name,value);  }

void alter_sorts(ClientInvoker* self,
          const boost::python::list& list,
          const std::string& attribute_name,
          bool recursive = true) {std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths);self->check(paths);self->alter_sort(paths,attribute_name,recursive); }
void alter_sort(ClientInvoker* self,
          const std::string& path,
          const std::string& attribute_name,
          bool recursive = true) {self->alter_sort(std::vector<std::string>(1,path),attribute_name,recursive );  }

void set_child_pid(ClientInvoker* self,int pid) { self->set_child_pid( boost::lexical_cast<std::string>(pid)); }

const std::vector<Zombie>& zombieGet(ClientInvoker* self,int pid) {
   self->zombieGet();
   return self->server_reply().zombies();
}

void zombieFobCli(ClientInvoker* self,const boost::python::list& list){std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths); self->zombieFobCliPaths(paths);}
void zombieFailCli(ClientInvoker* self,const boost::python::list& list){std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths); self->zombieFailCliPaths(paths);}
void zombieAdoptCli(ClientInvoker* self,const boost::python::list& list){std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths); self->zombieAdoptCliPaths(paths);}
void zombieBlockCli(ClientInvoker* self,const boost::python::list& list){std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths); self->zombieBlockCliPaths(paths);}
void zombieRemoveCli(ClientInvoker* self,const boost::python::list& list){std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths); self->zombieRemoveCliPaths(paths);}
void zombieKillCli(ClientInvoker* self,const boost::python::list& list){std::vector<std::string> paths;BoostPythonUtil::list_to_str_vec(list,paths); self->zombieKillCliPaths(paths);}

void export_Client()
{
 	class_<ClientInvoker, boost::noncopyable >("Client",ClientDoc::class_client())
   .def( init<std::string>() /* host:port      */)
   .def( init<std::string,std::string>() /* host, port      */)
   .def( init<std::string,int>()         /* host, port(int) */)
   .def("version",          &version,        "Returns the current client version")
   .def("server_version",   &server_version, "Returns the server version, can throw for old servers, that did not implement this request.")
   .def("set_host_port",    &ClientInvoker::set_host_port,  ClientDoc::set_host_port())
   .def("set_host_port",    &set_host_port )
   .def("set_host_port",    &set_host_port_1 )
   .def("get_host",         &ClientInvoker::host, return_value_policy<copy_const_reference>(), "Return the host, assume set_host_port() has been set, otherwise return localhost" )
   .def("get_port",         &ClientInvoker::port, return_value_policy<copy_const_reference>(), "Return the port, assume set_host_port() has been set. otherwise returns 3141" )
   .def("set_retry_connection_period",&ClientInvoker::set_retry_connection_period,  ClientDoc::set_retry_connection_period())
   .def("set_connection_attempts",&ClientInvoker::set_connection_attempts,  ClientDoc::set_connection_attempts())
   .def("get_defs",         &ClientInvoker::defs,           ClientDoc::get_defs())
   .def("reset",            &ClientInvoker::reset,          "reset client definition, and handle number")
	.def("in_sync",          &ClientInvoker::in_sync,        ClientDoc::in_sync())
#ifdef DEBUG
 	.def("get_log" ,         &get_log,                       return_value_policy<copy_const_reference>(),ClientDoc::get_log())
#endif
 	.def("new_log",          &ClientInvoker::new_log, (bp::arg("path")=""),ClientDoc::new_log())
 	.def("clear_log",        &ClientInvoker::clearLog,                     ClientDoc::clear_log())
 	.def("flush_log",        &ClientInvoker::flushLog,                     ClientDoc::flush_log())
 	.def("log_msg",          &ClientInvoker::logMsg,                       ClientDoc::log_msg())
 	.def("restart_server",   &ClientInvoker::restartServer,   ClientDoc::restart_server())
	.def("halt_server",      &ClientInvoker::haltServer,      ClientDoc::halt_server())
	.def("shutdown_server",  &ClientInvoker::shutdownServer,  ClientDoc::shutdown_server())
   .def("terminate_server", &ClientInvoker::terminateServer, ClientDoc::terminate_server())
   .def("wait_for_server_reply",&ClientInvoker::wait_for_server_reply,(bp::arg("time_out")=60), ClientDoc::wait_for_server_reply())
	.def("load" ,            &ClientInvoker::loadDefs,(bp::arg("path_to_defs"), bp::arg("force")=false,bp::arg("check_only")=false),ClientDoc::load_defs())
	.def("load" ,            &ClientInvoker::load,(bp::arg("defs"), bp::arg("force")=false),ClientDoc::load())
	.def("get_server_defs",  &ClientInvoker::getDefs,         ClientDoc::get_server_defs())
   .def("sync_local",       &ClientInvoker::sync_local,(bp::arg("sync_suite_clock")=false),ClientDoc::sync())
 	.def("news_local",       &news_local,                     ClientDoc::news())
   .add_property("changed_node_paths",boost::python::range( &ClientInvoker::changed_node_paths_begin, &ClientInvoker::changed_node_paths_end),ClientDoc::changed_node_paths())
	.def("suites" ,          &suites,                         ClientDoc::suites())
	.def("ch_register",      &ch_register,                    ClientDoc::ch_register())
   .def("ch_suites",        &ch_suites,                      ClientDoc::ch_suites())
   .def("ch_handle",        &ClientInvoker::client_handle,   ClientDoc::ch_register())
	.def("ch_drop",          &ClientInvoker::ch_drop,         ClientDoc::ch_drop())
   .def("ch_drop",          &ClientInvoker::ch1_drop)
   .def("ch_drop_user",     &ClientInvoker::ch_drop_user,    ClientDoc::ch_drop_user())
	.def("ch_add",           &ch_add,                         ClientDoc::ch_add())
	.def("ch_add",           &ch1_add )
	.def("ch_remove",        &ch_remove,                      ClientDoc::ch_remove())
	.def("ch_remove",        &ch1_remove)
	.def("ch_auto_add",      &ClientInvoker::ch_auto_add,     ClientDoc::ch_auto_add())
	.def("ch_auto_add",      &ClientInvoker::ch1_auto_add)
   .def("checkpt",          &ClientInvoker::checkPtDefs,     (bp::arg("mode")=ecf::CheckPt::UNDEFINED, bp::arg("check_pt_interval")=0,bp::arg("check_pt_save_alarm_time")=0), ClientDoc::checkpt())
	.def("restore_from_checkpt",&ClientInvoker::restoreDefsFromCheckPt,ClientDoc::restore_from_checkpt())
	.def("reload_wl_file" ,  &ClientInvoker::reloadwsfile,    ClientDoc::reload_wl_file())
   .def("requeue" ,         &requeue,                        (bp::arg("abs_node_path"), bp::arg("option")=""), ClientDoc::requeue())
   .def("requeue" ,         &requeues,                       (bp::arg("paths"), bp::arg("option")="") )
   .def("free_trigger_dep", &free_trigger_dep,               ClientDoc::free_trigger_dep())
   .def("free_trigger_dep", &free_trigger_dep1 )
   .def("free_date_dep",    &free_date_dep,                  ClientDoc::free_date_dep())
   .def("free_date_dep",    &free_date_dep1 )
   .def("free_time_dep",    &free_time_dep,                  ClientDoc::free_time_dep())
   .def("free_time_dep",    &free_time_dep1)
   .def("free_all_dep",     &free_all_dep,                   ClientDoc::free_all_dep())
   .def("free_all_dep",     &free_all_dep1)
	.def("ping" ,            &ClientInvoker::pingServer,      ClientDoc::ping())
   .def("stats" ,           &stats,                          ClientDoc::stats())
   .def("stats_reset" ,     &stats_reset,                    ClientDoc::stats_reset())
   .def("get_file" ,        &get_file,                       return_value_policy<copy_const_reference>(), ClientDoc::get_file())
   .def("get_file" ,        &get_file_1,                     return_value_policy<copy_const_reference>())
   .def("plug" ,            &ClientInvoker::plug,            ClientDoc::plug())
   .def("query" ,           &query,                          return_value_policy<copy_const_reference>(),ClientDoc::query())
   .def("alter" ,           &alters,(bp::arg("paths"),bp::arg("alter_type"),bp::arg("attribute_type"),bp::arg("name")="",bp::arg("value")=""), ClientDoc::alter())
   .def("alter" ,           &alter,(bp::arg("abs_node_path"),bp::arg("alter_type"),bp::arg("attribute_type"),bp::arg("name")="",bp::arg("value")=""))
   .def("sort_attributes" , &alter_sort,(bp::arg("abs_node_path"),bp::arg("attribute_name"),bp::arg("recursive")=true))
   .def("sort_attributes" , &alter_sorts,(bp::arg("paths"),bp::arg("attribute_name"),bp::arg("recursive")=true))
   .def("force_event" ,     &force_event,                    ClientDoc::force_event())
   .def("force_event" ,     &force_events)
   .def("force_state" ,     &force_state,                    ClientDoc::force_state())
   .def("force_state" ,     &force_states)
   .def("force_state_recursive",&force_state_recursive,      ClientDoc::force_state_recursive())
   .def("force_state_recursive",&force_states_recursive)
   .def("replace" ,         &ClientInvoker::replace,         ClientDoc::replace())
   .def("replace" ,         &ClientInvoker::replace_1)
   .def("replace" ,         &replace_1     )
   .def("replace" ,         &replace_2     )
	.def("order" ,           &order,           ClientDoc::order())
	.def("group" ,           &ClientInvoker::group,           ClientDoc::group())
   .def("begin_suite" ,     &ClientInvoker::begin,           (bp::arg("suite_name"),bp::arg("force")=false), ClientDoc::begin_suite())
   .def("begin_all_suites", &ClientInvoker::begin_all_suites,(bp::arg("force")=false), ClientDoc::begin_all())
   .def("job_generation",   &ClientInvoker::job_gen,         ClientDoc::job_gen())
   .def("run" ,             &run,                            ClientDoc::run())
   .def("run" ,             &runs)
   .def("check" ,           &check,  return_value_policy<copy_const_reference>(), ClientDoc::check())
   .def("check" ,           &checks, return_value_policy<copy_const_reference>())
   .def("kill" ,            &do_kill,         ClientDoc::kill())
   .def("kill" ,            &do_kills)
   .def("status" ,          &the_status,          ClientDoc::status())
   .def("status" ,          &statuss)
   .def("suspend" ,         &suspend,         ClientDoc::suspend())
   .def("suspend" ,         &suspends)
   .def("resume" ,          &resume,          ClientDoc::resume())
   .def("resume" ,          &resumes)
   .def("archive" ,         &archive,          ClientDoc::archive())
   .def("archive" ,         &archives)
   .def("restore" ,         &restore,          ClientDoc::restore())
   .def("restore" ,         &restores)
   .def("delete" ,          &ClientInvoker::delete_node,     (bp::arg("abs_node_path"),bp::arg("force")=false), ClientDoc::delete_node())
   .def("delete" ,          &delete_node,                    (bp::arg("paths"),bp::arg("force")=false))
   .def("delete_all",       &ClientInvoker::delete_all,      (bp::arg("force")=false),                          ClientDoc::delete_all())
   .def("debug_server_on",  &ClientInvoker::debug_server_on,  "Enable server debug, Will dump to standard out on server host.")
   .def("debug_server_off", &ClientInvoker::debug_server_off, "Disable server debug")

   .def("zombie_get",       &zombieGet, return_value_policy<copy_const_reference>())
   .def("zombie_fob",       &ClientInvoker::zombieFobCli )
   .def("zombie_fail",      &ClientInvoker::zombieFailCli )
   .def("zombie_adopt",     &ClientInvoker::zombieAdoptCli )
   .def("zombie_block",     &ClientInvoker::zombieBlockCli )
   .def("zombie_remove",    &ClientInvoker::zombieRemoveCli )
   .def("zombie_kill",      &ClientInvoker::zombieKillCli )
   .def("zombie_fob",       &zombieFobCli )
   .def("zombie_fail",      &zombieFailCli )
   .def("zombie_adopt",     &zombieAdoptCli )
   .def("zombie_block",     &zombieBlockCli )
   .def("zombie_remove",    &zombieRemoveCli )
   .def("zombie_kill",      &zombieKillCli )

   .def("set_child_path",      &ClientInvoker::set_child_path ,   "Set the path to the task, obtained from server using %ECF_NAME%")
   .def("set_child_password",  &ClientInvoker::set_child_password,"Set the password, needed for authentication, provided by the server using %ECF_PASS%")
   .def("set_child_pid",       &ClientInvoker::set_child_pid,     "Set the process id of this job" )
   .def("set_child_pid",       &set_child_pid,                    "Set the process id of this job" )
   .def("set_child_try_no",    &ClientInvoker::set_child_try_no,  "Set the try no, i.e the number of times this job has run, obtained from the server, using %ECF_TRYNO%")
   .def("set_child_timeout",   &ClientInvoker::set_child_timeout, "Set timeout if child can not connect to server, default is 24 hours. The input is required to be in seconds")
   .def("set_zombie_child_timeout",&ClientInvoker::set_zombie_child_timeout, "Set timeout for zombie child commands,that can not connect to server, default is 24 hours. The input is required to be in seconds")
   .def("child_init",          &ClientInvoker::child_init,    "Child command,notify server job has started")
   .def("child_abort",         &ClientInvoker::child_abort,  (bp::arg("reason")=""), "Child command,notify server job has aborted, can provide an optional reason")
   .def("child_event",         &ClientInvoker::child_event,   "Child command,notify server event occurred, requires the event name")
   .def("child_meter",         &ClientInvoker::child_meter,   "Child command,notify server meter changed, requires meter name and value")
   .def("child_label",         &ClientInvoker::child_label,   "Child command,notify server label changed, requires label name, and new value")
   .def("child_wait",          &ClientInvoker::child_wait,    "Child command,wait for expression to come true")
   .def("child_queue",         &ClientInvoker::child_queue,(bp::arg("queue_name"),bp::arg("path_to_node_with_queue")=""),"Child command,queue the next step, return the next step as a string. requires queue name, and optionally path to node with the queue")
   .def("child_complete",      &ClientInvoker::child_complete,"Child command,notify server job has complete")
	;

	class_<WhyCmd,  boost::noncopyable >( "WhyCmd",
	         "The why command reports, the reason why a node is not running.\n\n"
	         "It needs the  definition structure and the path to node\n"
	         "\nConstructor::\n\n"
	         "   WhyCmd(defs, node_path)\n"
	         "      defs_ptr  defs   : pointer to a definition structure\n"
	         "      string node_path : The node path\n\n"
	         "\nExceptions:\n\n"
	         "- raises RuntimeError if the definition is empty\n"
	         "- raises RuntimeError if the node path is empty\n"
	         "- raises RuntimeError if the node path can not be found in the definition\n"
	         "\nUsage::\n\n"
	         "   try:\n"
	         "      ci = Client()\n"
	         "      ci.sync_local()\n"
	         "      ask = WhyCmd(ci.get_defs(),'/suite/family')\n"
	         "      print ask.why()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
	         ,
			init<defs_ptr,std::string >()
	)
	.def("why", &WhyCmd::why, "returns a '/n' separated string, with reasons why node is not running")
 	;

	class_<UrlCmd,  boost::noncopyable >( "UrlCmd",
	         "Executes a command ECF_URL_CMD to display a url.\n\n"
	         "It needs the definition structure and the path to node.\n"
	         "\nConstructor::\n\n"
	         "   UrlCmd(defs, node_path)\n"
	         "      defs_ptr  defs   : pointer to a definition structure\n"
	         "      string node_path : The node path.\n\n"
	         "\nExceptions\n\n"
	         "- raises RuntimeError if the definition is empty\n"
	         "- raises RuntimeError if the node path is empty\n"
	         "- raises RuntimeError if the node path can not be found in the definition\n"
	         "- raises RuntimeError if ECF_URL_CMD not defined or if variable substitution fails\n"
	         "\nUsage:\n"
	         "Lets assume that the server has the following definition::\n\n"
	         "   suite s\n"
	         "      edit ECF_URL_CMD  \"${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'\"\n"
	         "      edit ECF_URL_BASE \"http://www.ecmwf.int\"\n"
	         "      family f\n"
	         "         task t1\n"
	         "            edit ECF_URL \"publications/manuals/ecflow\"\n"
	         "         task t2\n"
	         "            edit ECF_URL index.html\n\n"
	         "::\n\n"
            "   try:\n"
	         "      ci = Client()\n"
            "      ci.sync_local()\n"
            "      url = UrlCmd(ci.get_defs(),'/suite/family/task')\n"
	         "      print url.execute()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
  			,
			init<defs_ptr,std::string >()
	)
	.def("execute", &UrlCmd::execute, "Displays url in the chosen browser")
 	;
}
