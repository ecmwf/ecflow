/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #89 $ 
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

#include "ClientDoc.hpp"

const char* ClientDoc::class_client(){
   return
            "Class client provides an interface to communicate with the :term:`ecflow_server`.::\n\n"
            "   Client(\n"
            "      string host, # The server name. Can not be empty.\n"
            "      string port  # The port on the server, must be unique to the server\n"
            "   )\n\n"
            "   Client(\n"
            "      string host, # The server name. Can not be empty.\n"
            "      int port     # The port on the server, must be unique to the server\n"
            "   )\n\n"
            "   Client(\n"
            "      string host_port, # Expect's <host>:<port>\n"
            "   )\n\n"
            "The client reads in the following environment variables.\n"
            "For child commands,(i.e these are commands called in the .ecf/jobs files), these variables are used.\n"
            "For the python interface these environment variable are not really applicable but documented for completeness:\n\n"
            "* ECF_NAME    <string> : Full path name to the task\n"
            "* ECF_PASS    <string> : The jobs password, allocated by server, then used by server to authenticate client request\n"
            "* ECF_TRYNO      <int> : The number of times to start a job if it aborts\n"
            "* ECF_TIMEOUT   <int>  : Max time in seconds for client to deliver message to main server\n"
            "* ECF_HOSTFILE<string> : File that lists alternate hosts to try, if connection to main host fails\n"
            "* ECF_DENIED     <any> : Provides a way for child to exit with an error, if server denies connection.\n"
            "                         Avoids 24hr wait. Note: when you have hundreds of tasks, using this approach\n"
            "                         requires a lot of manual intervention to determine job status\n"
            "* NO_ECF         <any> : If set exit's immediately with success. Used to test jobs without communicating with server\n\n"
            "The following environment variables are used by the python interface and child commands\n\n"
            "* ECF_NODE  <string>   : The host name of the main server. defaults to 'localhost'\n"
            "* ECF_PORT  <int>      : The TCP/IP port to call on the server. Must be unique to a server\n\n"
            "The ECF_NODE and ECF_PORT can be overridden by using the Constructor or set_host_port() member function.\n"
            "For optimal usage it is best to reuse the same Client rather than recreating for each client server interaction\n"
            "By default the Client interface will throw exceptions for error's.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client(\"localhost:3150\")   # for errors will throw RuntimeError\n"
            "       ci.terminate_server()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
            ;
}

const char* ClientDoc::set_host_port(){
   return
            "Override the default(localhost and port 3141) and environment setting(ECF_NODE and ECF_PORT)\n"
            "and set it explicitly::\n\n"
            "   set_host_port(\n"
            "      string host, # The server name. Can not be empty.\n"
            "      string port  # The port on the server, must be unique to the server\n"
            "   )\n\n"
            "   set_host_port(\n"
            "      string host, # The server name. Can not be empty.\n"
            "      int port     # The port on the server, must be unique to the server\n"
            "   )\n\n"
            "   set_host_port(\n"
            "      string host_port, # Expect's <host>:<port>\n"
            "   )\n\n"
            "Exceptions:\n\n"
            "- Raise a RuntimeError if the host or port is empty\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client();\n"
            "       ci.set_host_port(\"localhost\",\"3150\")\n"
            "       ci.set_host_port(\"avi\",3150)\n"
            "       ci.set_host_port(\"avi:3150\")\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
            ;
}

const char* ClientDoc::set_retry_connection_period() {
   return
            "Set the sleep period between connection attempts\n\n"
            "Whenever there is a connection failure we wait a number of seconds before trying again.\n"
            "i.e. to get round glitches in the network.\n"
            "For the ping command this is hard wired as 1 second.\n"
            "This wait between connection attempts can be configured here.\n"
            "i.e This could be reduced to increase responsiveness.\n"
            "Default: In debug this period is 1 second and in release mode 10 seconds::\n\n"
            "   set_retry_connection_period(\n"
            "      int period # must be an integer >= 0\n"
            "   )\n\n"
            "Exceptions:\n\n"
            "- None\n"
            "\nUsage::\n\n"
            "   ci = Client()\n"
            "   ci.set_connection_attempts(3)     # make 3 attempts for server connection\n"
            "   ci.set_retry_connection_period(1) # wait 1 second between each attempt\n"
            ;
}

const char* ClientDoc::set_connection_attempts() {
   return
            "Set the number of times to connect to :term:`ecflow_server`, in case of connection failures\n\n"
            "The period between connection attempts is handled by Client.set_retry_connection_period().\n"
            "If the network is unreliable the connection attempts can be be increased, likewise\n"
            "when the network is stable this number could be reduced to one.\n"
            "This can increase responsiveness and reduce latency.\n"
            "Default value is set as 2.\n"
            "Setting a value less than one is ignored, will default to 1 in this case::\n\n"
            "   set_connection_attempts(\n"
            "      int attempts # must be an integer >= 1\n"
            "   )\n"
            "\nExceptions:\n\n"
            "- None\n"
            "\nUsage::\n\n"
            "   ci = Client()\n"
            "   ci.set_connection_attempts(3)     # make 3 attempts for server connection\n"
            "   ci.set_retry_connection_period(1) # wait 1 second between each attempt\n"
           ;
}

const char* ClientDoc::get_defs(){
   return
            "Returns the :term:`suite definition` stored on the Client.\n\n"
            "Use :py:class:`ecflow.Client.sync_local()` to retrieve the definition from the server first.\n"
            "The definition is *retained* in memory until the next call to sync_local().\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()         # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.sync_local()       # get the definition from the server and store on 'ci'\n"
            "       print ci.get_defs()   # print out definition stored in the client\n"
            "       print ci.get_defs()   # print again, this shows that defs is retained on ci\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::get_log()
{
   return
            "Request the :term:`ecflow_server` to return the log file contents as a string\n\n"
            "Use with caution as the returned string could be several megabytes.\n"
            "Only enabled in the debug build of ECF.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()          # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       print ci.get_log()     # get the log file from the server\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::new_log()
{
   return
            "Request the :term:`ecflow_server` to use the path provided, as the new log file\n\n"
            "The old log file is released.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()               # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.new_log('/path/log.log') # use '/path/log,log' as the new log file\n"
            "                                   # To keep track of log file Can change ECF_LOG\n"
            "       ci.alter('\','change','variable','ECF_LOG','/new/path.log')\n"
            "       ci.new_log()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
           ;
}

const char* ClientDoc::clear_log()
{
   return
            "Request the :term:`ecflow_server` to clear log file.\n"
            "Log file will be empty after this call.\n\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.clear_log()   # log file is now empty\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::flush_log()
{
   return
            "Request the :term:`ecflow_server` to flush and then close log file\n\n"
            "It is best that the server is :term:`shutdown` first, as log file will be reopened\n"
            "whenever a command wishes to log any changes.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.flush_log()   # Log can now opened by external program\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::log_msg()
{
   return
            "Request the :term:`ecflow_server` writes a string message to the log file.\n\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()             # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.log_msg(\"A message\") # Write message to log file\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::restart_server()   {
   return
            "Restart the :term:`ecflow_server`\n\n"
            "Start job scheduling, communication with jobs, and respond to all requests.\n"
            "See :term:`server states`\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()            # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.retstart_server()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}
const char* ClientDoc::halt_server()      {
   return
            "Halt the :term:`ecflow_server`\n\n"
            "Stop server communication with jobs, and new job scheduling, and stops check pointing.\n"
            "See :term:`server states`\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()            # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.halt_server()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}
const char* ClientDoc::shutdown_server()  {
   return
            "Shut down the :term:`ecflow_server`\n\n"
            "Stop server from scheduling new jobs.\n"
            "See :term:`server states`\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()            # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.shutdown_server()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::terminate_server() {
   return   "Terminate the :term:`ecflow_server`::\n\n"
            "   try:\n"
            "       ci = Client()            # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.terminate_server()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::wait_for_server_reply() {
   return   "Wait for a response from the :term:`ecflow_server`::\n\n"
            "   void wait_for_server_reply(\n"
            "      int time_out     : (default = 60) \n"
            "   )\n\n"
            "This is used to check if server has started. Typically for tests.\n"
            "Returns true if server(ping) replies before time out, otherwise false\n"
            "\nUsage::\n\n"
            "   ci = Client()   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "   if ci.wait_for_server_reply(30):\n"
            "      print 'Server is alive'\n"
            "   else:\n"
            "      print 'Timed out after 30 second wait for server response.?'\n"
            ;
}

const char* ClientDoc::load_defs(){
   return
            "Load a :term:`suite definition` given by the file_path argument into the :term:`ecflow_server`::\n\n"
            "   void load(\n"
            "      string file_path     : path name to the definition file\n"
            "      [(bool)force=False]  : If true overwrite suite of same name\n"
            "   )\n\n"
            "By default throws a RuntimeError exception for errors.\n"
            "If force is not used and :term:`suite` of the same name already exists in the server,\n"
            "then a error is thrown\n"
            "\nUsage::\n\n"
            "   defs_file = \"Hello.def\" \n"
            "   defs = Defs()\n"
            "   suite = def.add_suite(\"s1\")\n"
            "   family = suite.add_family(\"f1\")\n"
            "   for i in [ \"_1\", \"_2\", \"_3\" ]:\n"
            "      family.add_task( \"t\" + i )\n"
            "   defs.save_as_defs(defs_file)  # write out in memory defs into the file 'Hello.def'\n"
            "   ...\n"
            "   try:\n"
            "       ci = Client()       # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.load(defs_file)  # open and parse defs file, and load into server.\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::load(){
   return
            "Load a in memory :term:`suite definition` into the :term:`ecflow_server`::\n\n"
            "   void load(\n"
            "      Defs defs           : A in memory definition\n"
            "      [(bool)force=False] : for true overwrite suite of same name\n"
            "   )\n\n"
            "If force is not used and :term:`suite` already exists in the server, then a error is thrown.\n"
            "\nUsage::\n\n"
            "   defs = Defs(\"hello.def\")\n"
            "   suite = defs.add_suite(\"s1\")\n"
            "   family = suite.add_family(\"f1\")\n"
            "   for i in [ \"_1\", \"_2\", \"_3\" ]: \n"
            "       family.add_task( Task( \"t\" + i) )\n"
            "   ...\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.load(defs)    # Load in memory defs, into the server\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::get_server_defs() {
   return
            "Get all suite Node tree's from the :term:`ecflow_server`.\n\n"
            "The definition is *retained* in memory until the next call to get_server_defs().\n"
            "This is important since get_server_defs() could return several megabytes of data.\n"
            "Hence we only want to call it once, and then access it locally with get_defs().\n"
            "If you need to access the server definition in a loop use :py:class:`ecflow.Client.sync_local` instead\n"
            "since this is capable of returning incremental changes, and thus considerably\n"
            "reducing the network load.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()         # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.get_server_defs()  # get the definition from the server and store on 'ci'\n"
            "       print ci.get_defs()   # print out definition stored in the client\n"
            "       print ci.get_defs()   # print again, this shows that defs is retained on ci\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::sync() {
   return
            "Requests that :term:`ecflow_server` returns the full definition or incremental change made and applies them to the client Defs\n\n"
            "When there is a very large definition, calling :py:class:`ecflow.Client.get_server_defs` each time can be *very* expensive\n"
            "both in terms of memory, speed, and network bandwidth. The alternative is to call \n"
            "this function, which will get the incremental changes, and apply them local client :term:`suite definition`\n"
            "effectively synchronising the client and server Defs.\n"
            "If the period of time between two sync() calls is too long, then the full server definition\n"
            "is returned and assigned to the client Defs.\n"
            "We can determine if the changes were applied by calling in_sync() after the call to sync_local()::\n\n"
            "   void sync_local();                     # The very first call, will get the full Defs.\n"
            "\n"
            "Exceptions:\n\n"
            "- raise a RuntimeError if the delta change can not be applied.\n"
            "- this could happen if the client Defs bears no resemblance to server Defs\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                       # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.sync_local()                     # Very first call gets the full Defs\n"
            "       client_defs = ci.get_defs()         # End user access to the returned Defs\n"
            "       ... after a period of time\n"
            "       ci.sync_local()                     # Subsequent calls to sync_local() users the local Defs to sync incrementally\n"
            "       if ci.in_sync():                    # returns true server changed and changes applied to client\n"
            "          print 'Client is now in sync with server'\n"
            "       client_defs = ci.get_defs()         # End user access to the returned Defs\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            "\n"
            "Calling sync_local() is considerably faster than calling get_server_defs() for large Definitions"
            ;
}

const char* ClientDoc::in_sync() {
   return
            "Returns true if the definition on the client is in sync with the :term:`ecflow_server`\n\n"
            "Calling in_sync() is **only** valid after a call to sync_local().\n"
            "\nUsage::\n\n"
            "   try:\n"
            "      ci = Client()                       # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "      ci.sync_local()                     # very first call gets the full Defs\n"
            "      client_defs = ci.get_defs()         # End user access to the returned Defs\n"
            "      ... after a period of time\n"
            "      ci.sync_local()                     # Subsequent calls to sync_local() users the local Defs to sync incrementally\n"
            "      if ci.in_sync():                    # returns true  changed and changes applied to client\n"
            "         print 'Client is now in sync with server'\n"
            "      client_defs = ci.get_defs()         # End user access to the returned Defs\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
           ;
}

const char* ClientDoc::news() {
   return
            "Query the :term:`ecflow_server` to detect any changes.\n\n"
            "This returns a simple bool, if there has been changes, the user should call :py:class:`ecflow.Client.sync_local`.\n"
            "This will bring the client in sync with changes in the server. If sync_local() is not called\n"
            "then calling news_local() will always return true.\n"
            "news_local() uses the definition stored on the client::\n\n"
            "   bool news_local()\n"
            "\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                  # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       if ci.news_local():            # has the server changed\n"
            "          print 'Server Changed'      # server changed bring client in sync with server\n"
            "          ci.sync_local()             # get the full definition from the server if first time\n"
            "                                      # otherwise apply incremental changes to Client definition,\n"
            "                                      # bringing it in sync with the server definition\n"
            "          print ci.get_defs()         # print the synchronised definition. Should be same as server\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::changed_node_paths() {
   return
            "After a call to sync_local() we can access the list of nodes that changed\n\n"
            "The returned list consists of node paths. *IF* the list is empty assume that\n"
            "whole definition changed. This should be expected after the first call to sync_local()\n"
            "since that always retrieves the full definition from the server::\n\n"
            "   void changed_node_paths()\n"
            "\n"
            "\nUsage::\n\n"
            "try:\n"
            "   ci = Client()                          # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "   if ci.news_local():                    # has the server changed\n"
            "      print 'Server Changed'              # server changed bring client in sync with server\n"
            "      ci.sync_local()                     # get the full definition from the server if first time\n"
            "                                          # otherwise apply incremental changes to Client definition,\n"
            "                                          # bringing it in sync with the server definition\n"
            "      defs = ci.get_defs()                # get the updated/synchronised definition\n"
            "      for path in ci.changed_node_paths():\n"
            "         print path;\n"
            "         if path == '/':                 # path '/' represent change to server node/defs\n"
            "            print 'defs changed'         # defs state change or user variables changed\n"
            "         else:\n"
            "            node = defs.find_abs_node_path()\n"
            "\n"
            "      # if changed_node_paths is empty, then assume entire definition changed\n"
            "      print defs                         # print the synchronised definition. Should be same as server\n"
            "except RuntimeError, e:\n"
            "  print str(e)\n"
            ;
}


const char* ClientDoc::checkpt(){
   return
            "Request the :term:`ecflow_server` :term:`check point` s the definition held in the server immediately\n\n"
            "This effectively saves the definition held in the server to disk,\n"
            "in a platform independent manner. This is the default when no arguments are specified.\n"
            "The saved file will include node state, passwords, etc.\n"
            "The default file name is <host>.<port>.ecf.check and is saved in ECF_HOME directory.\n"
            "The :term:`check point` file name can be overridden via ECF_CHECK server environment variable.\n"
            "The back up :term:`check point` file name can be overridden via ECF_CHECKOLD server environment variable::\n\n"
            "   void checkpt(\n"
            "     [(CheckPt::Mode)mode=CheckPt.UNDEFINED]\n"
            "                         : Must be one of [ NEVER, ON_TIME, ALWAYS, UNDEFINED ]\n"
            "                           NEVER  :  Never check point the definition in the server\n"
            "                           ON_TIME:  Turn on automatic check pointing at interval stored on server\n"
            "                                     or with interval specified as the second argument\n"
            "                           ALWAYS:   Check point at any change in node tree, *NOT* recommended for large definitions\n"
            "                           UNDEFINED:The default, which allows for immediate check pointing, or alarm setting\n"
            "     [(int)interval=120] : This specifies the interval in seconds when server should automatically check pt.\n"
            "                           This will only take effect if mode is on_time/CHECK_ON_TIME\n"
            "                           Should ideally be a value greater than 60 seconds, default is 120 seconds\n"
            "     [(int)alarm=30]     : Specifies check pt save alarm time. If saving the check pt takes longer than\n"
            "                           the alarm time, then the late flag is set on the server.\n"
            "                           This flag will need to be cleared manually.\n"
            "   )\n\n"
            "Note: When the time taken to save the check pt is excessive, it can interfere with job scheduling.\n\n"
            "It may be an indication of the following:\n\n"
            "* slow disk\n"
            "* file system full\n"
            "* The definition is very large and needs to split\n\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                      # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.checkpt()                       # Save the definition held in the server to disk\n"
            "       ci.checkpt(CheckPt.NEVER)          # Switch off check pointing\n"
            "       ci.checkpt(CheckPt.ON_TIME)        # Start automatic check pointing at the interval stored in the server\n"
            "       ci.checkpt(CheckPt.ON_TIME,180)    # Start automatic check pointing every 180 seconds\n"
            "       ci.checkpt(CheckPt.ALWAYS)         # Check point at any state change in node tree. *not* recommended for large defs\n"
            "       ci.checkpt(CheckPt.UNDEFINED,0,35) # Change check point save time alarm to 35 seconds\n"
            "                                          # With these arguments mode and interval remain unchanged\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}


const char* ClientDoc::restore_from_checkpt() {
   return
            "Request the :term:`ecflow_server` loads the :term:`check point` file from disk\n\n"
            "The server will first try to open file at ECF_HOME/ECF_CHECK if that fails it will\n"
            "then try path ECF_HOME/ECF_CHECKOLD.\n"
            "An error is returned if the server has not been :term:`halted` or contains a :term:`suite definition`\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()             # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.halt_server()          # server must be halted, otherwise restore_from_checkpt will throw\n"
            "       ci.restore_from_checkpt() # restore the definition from the check point file\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}


const char* ClientDoc::reload_wl_file(){
   return
            "Request that the :term:`ecflow_server` reload the white list file.\n\n"
            "The white list file if present, can be used to control who has read/write\n"
            "access to the :term:`ecflow_server`::\n\n"
            "   void reload_wl_file()\n\n"
            "Usage::\n\n"
            "   try:\n"
            "       ci = Client()            # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.reload_wl_file()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::run(){
   return
            "Immediately run the jobs associated with the input :term:`node`.\n\n"
            "Ignore :term:`trigger` s, :term:`limit` s, :term:`suspended`, :term:`time` or :term:`date` dependencies,\n"
            "just run the :term:`task`.\n"
            "When a job completes, it may be automatically re-queued if it has\n"
            "multiple time :term:`dependencies`. In the specific case where a :term:`task` has a SINGLE\n"
            "time dependency and we want to avoid re running the :term:`task` then\n"
            "a flag is set so that it is not automatically re-queued when set to :term:`complete`.\n"
            "The flag is applied up the :term:`node` hierarchy until we reach a node with a :term:`repeat`\n"
            "or :term:`cron` attribute. This behaviour allow :term:`repeat` values to be incremented interactively.\n"
            "A :term:`repeat` attribute is incremented when all the child nodes are :term:`complete`\n"
            "in this case the child nodes are automatically re-queued\n::\n\n"
            "   void run(\n"
            "      string absolute_node_path : Path name to node. If the path is suite/family will recursively\n"
            "                                  run all child tasks\n"
            "      [(bool)force=False]       : If true, run even if there are nodes that are active or submitted.\n"
            "   )\n"
            "   void run(\n"
            "      list  paths               : List of paths. If the path is suite/family will recursively run all child tasks\n"
            "      [(bool)force=False]       : If true, run even if there are nodes that are active or submitted.\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                          # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.run('/s1')                          # run all tasks under suite /s1\n"
            "\n"
            "       path_list = ['/s1/f1/t1','/s2/f1/t2']\n"
            "       ci.run(path_list)                      # run all tasks specified in the paths\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            "\nEffect::\n\n"
            "   Lets see the effect of run command on the following defs::\n\n"
            "   suite s1\n"
            "      task t1; time 10:00             # will complete straight away\n"
            "      task t2; time 10:00 13:00 01:00 # will re-queue 3 times and complete on fourth run\n\n"
            "In the last case (task t2) after each run the next time slot is incremented.\n"
            "This can be seen by calling the Why command."
            ;
}

const char* ClientDoc::requeue(){

   return
            "Re queues the specified :term:`node` (s)::\n\n"
            "   void requeue(\n"
            "      list paths     : A list of paths. Node paths must begin with a leading '/' character\n"
            "      [(str)option=''] : option = ('' | 'abort' | 'force')\n"
            "          ''   : empty string, the default, re-queue the node\n"
            "          abort: means re-queue only aborted tasks below node\n"
            "          force: means re-queueing even if there are nodes that are active or submitted\n"
            "   )\n"
            "   void requeue(\n"
            "      string absolute_node_path : Path name to node\n"
            "      [(string)option='']       : option = ('' | 'abort' | 'force')\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.requeue('/s1','abort')       # re-queue aborted tasks below suite /s1\n"
            "\n"
            "       path_list = ['/s1/f1/t1','/s2/f1/t2']\n"
            "       ci.requeue(path_list)\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::free_trigger_dep(){
   return
            "Free :term:`trigger` :term:`dependencies` for a :term:`node`::\n\n"
            "   void free_trigger_dep(\n"
            "      string absolute_node_path : Path name to node\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()         # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.free_trigger_dep('/s1')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::free_date_dep(){
   return
            "Free :term:`date` :term:`dependencies` for a :term:`node`::\n\n"
            "   void free_date_dep(\n"
            "      string absolute_node_path : Path name to node\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.free_date_dep('/s1')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::free_time_dep(){
   return
            "Free all time :term:`dependencies`. i.e :term:`time`, :term:`day`, :term:`today`, :term:`cron`::\n\n"
            "   void free_time_dep(\n"
            "      string absolute_node_path : Path name to node\n"
            "   )\n\n"
            "After freeing the time related dependencies (i.e time,today,cron)\n"
            "the next time slot will be missed.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.free_time_dep('/s1')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::free_all_dep(){
   return
            "Free all :term:`trigger`, :term:`date` and all time(:term:`day`, :term:`today`, :term:`cron`,etc) :term:`dependencies`::\n\n"
            "   void free_all_dep(\n"
            "      string absolute_node_path : Path name to node\n"
            "   )\n\n"
            "After freeing the time related dependencies (i.e time,today,cron)\n"
            "the next time slot will be missed.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.free_all_dep('/s1')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::ping(){
   return
            "Checks if the :term:`ecflow_server` is running::\n\n"
            "   void ping()\n\n"
            "The default behaviour is to check on host 'localhost' and port 3141\n"
            "It should be noted that any Client function will fail if the server is\n"
            "is not running. Hence ping() is not strictly required. However its main\n"
            "distinction from other Client function is that it is quite fast.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client(\"localhost\",\"3150\")\n"
            "       ci.ping()\n"
            "       print \"------- Server already running------\"\n"
            "       do_something_with_server(ci)\n"
            "   except RuntimeError, e:\n"
            "       print \"------- Server *NOT* running------\" + str(e)\n"
            ;
}

const char* ClientDoc::stats(){
   return
            "Prints the :term:`ecflow_server` statistics to standard out::\n\n"
            "   void stats()\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()  # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.stats()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::stats_reset(){
   return
            "Resets the statistical data in the server::\n\n"
            "   void stats_reset()\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()  # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.stats_reset()\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::suites(){
   return
            "Returns a list strings representing the :term:`suite` names.\n\n"
            "   list(string) suites()\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()  # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       suites = ci.suites()\n"
            "       print suites\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::ch_register() {
   return
            "Register interest in a set of :term:`suite` s.\n\n"
            "If a definition has lots of suites, but the client is only interested in a small subset.\n"
            "Then using this command can reduce network bandwidth and synchronisation will be quicker.\n"
            "This command will create a client handle. This handle is held locally on the :py:class:`ecflow.Client`, and\n"
            "can be used implicitly by ch_drop(),ch_add(),ch_remove() and ch_auto_add().\n"
            "Registering a client handle affects the news() and sync() commands::\n\n"
            "   void ch_register(\n"
            "      bool auto_add_new_suites : true means add new suites to my list, when they are created\n"
            "      list suite_names         : should be a list of suite names, names not in the definition are ignored\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()\n"
            "       suite_names = [ 's1', 's2', 's3' ]\n"
            "       ci.ch_register(True,suite_names)    # register interest in suites s1,s2,s3 and any new suites\n"
            "       ci.ch_register(False,suite_names)   # register interest in suites s1,s2,s3 only\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
            "The client 'ci' will hold locally the client handle. Since we have made multiple calls to register\n"
            "a handle, the variable 'ci' will hold the handle for the last call only.\n"
            "The handle associated with the suite can be manually retrieved::\n\n"
            "   try:\n"
            "       ci = Client()\n"
            "       suite_names = [ 's1', 's2', 's3' ]\n"
            "       ci.ch_register(True,suite_names)    # register interest in suites s1,s2,s3 and any new suites\n"
            "       client_handle = ci.ch_handle()\n    # get the handle associated with last call to ch_register\n"
            "       ci.ch_drop( client_handle )         # de-register the handle\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
            ;
}

const char* ClientDoc::ch_suites() {
   return
            "Writes to standard out the list of registered handles and the suites they reference.\n\n";
}

const char* ClientDoc::ch_drop(){
   return
            "Drop/de-register the client handle.\n\n"
            "Client must ensure un-used handle are dropped otherwise they will stay, in the :term:`ecflow_server`::\n\n"
            "   void ch_drop(\n"
            "      int client_handle : The handle must be an integer that is > 0\n"
            "   )\n"
            "   void ch_drop()       : Uses the local handle stored on the client, from last call to ch_register()\n\n"
            "Exception:\n\n"
            "- RunTimeError thrown if handle has not been previously registered\n"
            "\nUsage::\n\n"
            "	try:\n"
            "      ci = Client()                      # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "      suites = [ 's1', 's2' ]\n"
            "      ci.ch_register(False, suites)\n"
            "      while( 1 ):\n"
            "         # get incremental changes to suites s1 & s2, uses data stored on ci/defs\n"
            "         ci.sync_local()                 # will only retrieve data for suites s1 & s2\n"
            "         update(ci.get_defs())\n"
            "	finally:\n"
            "      ci.ch_drop()\n"
            ;
}

const char* ClientDoc::ch_drop_user(){
   return
            "Drop/de-register all handles associated with user.\n\n"
            "Client must ensure un-used handle are dropped otherwise they will stay, in the :term:`ecflow_server`::\n\n"
            "   void ch_drop_user(\n"
            "        string user   # If empty string will drop current user\n"
            "   )\n\n"
            "Exception:\n\n"
            "- RunTimeError thrown if handle has not been previously registered\n"
            "\nUsage::\n\n"
            "  try:\n"
            "      ci = Client()       # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "      suites = [ 's1', 's2' ]\n"
            "      ci.ch_register(False, suites)\n"
            "      while( 1 ):\n"
            "         # get incremental changes to suites s1 & s2, uses data stored on ci/defs\n"
            "         ci.sync_local()                 # will only retrieve data for suites s1 & s2\n"
            "         update(ci.get_defs())\n"
            "  finally:\n"
            "      ci.ch_drop_user(\"\") # drop all handles associated with current user\n\n"
            ;
}

const char* ClientDoc::ch_add() {
   return
            "Add a set of suites, to an existing handle::\n\n"
            "  integer ch_add(\n"
            "     integer handle   : the handle obtained after ch_register\n"
            "     list suite_names : list of strings representing suite names\n"
            "  )\n"
            "  integer ch_add(\n"
            "     list suite_names : list of strings representing suite names\n"
            "  )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()        # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       suite_names = []\n"
            "       ci.ch_register(True,suite_names)  # register interest in any new suites\n"
            "       suite_names = [ 's1', 's2' ]\n"
            "       ci.ch_add(suite_names)            # add suites s1,s2 to the last added handle\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
            ;
}

const char* ClientDoc::ch_remove() {
   return
            "Remove a set of suites, from an existing handle::\n\n"
            "  integer ch_remove(\n"
            "     integer handle   : the handle obtained after ch_register\n"
            "     list suite_names : list of strings representing suite names\n"
            "  )\n"
            "  integer ch_remove(\n"
            "     list suite_names : list of strings representing suite names\n"
            "  )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                       # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       suite_names = [ 's1', 's2' , 's3']\n"
            "       ci.ch_register(True,suite_names)    # register interest in suites s1,s2,s3 and any new suites\n"
            "       suite_names = [ 's1' ]\n"
            "       ci.ch_remove( suite_names )         # remove suites s1 from the last added handle\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
            ;
}

const char* ClientDoc::ch_auto_add() {
   return
            "Change an existing handle so that new suites can be added automatically::\n\n"
            "   void ch_auto_add(\n"
            "      integer handle,         : the handle obtained after ch_register\n"
            "      bool auto_add_new_suite : automatically add new suites, this handle when they are created\n"
            "   )\n"
            "   void ch_auto_add(\n"
            "      bool auto_add_new_suite : automatically add new suites using handle on the client\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                       # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       suite_names = [ 's1', 's2' , 's3']\n"
            "       ci.ch_register(True,suite_names)    # register interest in suites s1,s2,s3 and any new suites\n"
            "       ci.ch_auto_add( False )             # disable adding newly created suites to my handle\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n\n"
            ;
}

const char* ClientDoc::get_file(){
   return
            "File command can be used to request the various file types associated with a :term:`node`\n\n"
            "This command defaults to returning a max of 10000 lines. This can be changed::\n\n"
            "   string get_file(\n"
            "      string absolute_node_path    : Path name to node\n"
            "      [(string)file_type='script'] : file_type = [ script<default> | job | jobout | manual | kill | stat ]\n"
            "      [(string)max_lines=\"10000\"]  : The number of lines in the file to return\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()        # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       for file in [ 'script', 'job', 'jobout', 'manual', 'kill', 'stat' ]:\n"
            "   	      print ci.get_file('/suite/f1/t1',file)  # make a request to the server\n"
            "   except RuntimeError, e:\n"
            "      print str(e)\n"
            ;
}

const char* ClientDoc::plug(){
   return
            "Plug command is used to move :term:`node` s\n\n"
            "The destination node can be on another :term:`ecflow_server`.\n"
            "In which case the destination path should be of the form '//<host>:<port>/suite/family/task::\n\n"
            "   void plug(\n"
            "      string source_absolute_node_path       : Path name to source node\n"
            "      string destination_absolute_node_path  : Path name to destination node. Note if only\n"
            "                                               '//host:port' is specified the whole suite can be moved\n"
            "   )\n\n"
            "By default throws a RuntimeError exception for errors.\n\n"
            "Exceptions can be raised because:\n\n"
            "- Source :term:`node` is in a :term:`active` or :term:`submitted` state.\n"
            "- Another user already has an lock.\n"
            "- source/destination paths do not exist on the corresponding servers\n"
            "- If the destination node path is empty, i.e. only host:port is specified,\n"
            "  then the source :term:`node` must correspond to a :term:`suite`.\n"
            "- If the source node is added as a child, then its name must be unique\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.plug('/suite','host3:3141')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::alter(){
   return
            "Alter command is used to change the attributes of a node::\n\n"
            "   void alter(\n"
            "      (list | string ) paths(s) : A single or list of paths. Path name to the node whose attributes are to be changed\n"
            "      string alter_type         : This must be one of [ 'add' | 'change' | 'delete' | 'set_flag' | 'clear_flag' ]\n"
            "      string attr_type          : This varies according to the 'alter_type'. valid strings are:\n"
            "         add    : [ variable,time, today, date, day, zombie]\n"
            "         delete : [ variable,time,today,date,day,cron,event,meter,label,trigger complete, repeat,limit,inlimit,limit_path,zombie]\n"
            "         change : [ variable,clock-type,clock-gain,event,meter,label,trigger,complete,repeat,limit-max,limit-value]\n"
            "         set_flag and clear_flag:\n"
            "                  [ force_aborted | user_edit | task_aborted | edit_failed | ecfcmd_failed | no_script | killed | \n"
            "                    migrated | late | message | complete | queue_limit | task_waiting | locked | zombie ]\n"
            "      string name               : used to locate the attribute, when multiple attributes of the same type,\n"
            "                                  optional for some.i.e. when changing, attributes like variable,meter,event,label,limits\n"
            "      string value              : Only used when 'changing' a attribute. provides a new value\n"
            "   )\n\n"
            "Exceptions can be raised because:\n\n"
            "- absolute_node_path does not exist.\n"
            "- parsing fails\n"
            "\n"
            "The following describes the parameters in more detail::\n\n"
            " add variable variable_name variable_value\n"
            " add time   format    # when format is +hh:mm | hh:mm | hh:mm(start) hh:mm(finish) hh:mm(increment)\n"
            " add today  format    # when format is +hh:mm | hh:mm | hh:mm(start) hh:mm(finish) hh:mm(increment)\n"
            " add date   format    # when format dd.mm.yyyy, can use '*' to indicate any day,month, or year\n"
            " add day    format    # when format is one of [ sunday,monday,tuesday,wednesday,friday,saturday ]\n"
            " add zombie format    # when format is one of <zombie-type>:<child>:<server-action>|<client-action>:<zombie-lifetime>\n"
            "                      #  <zombie-type> := [ user | ecf | path ]\n"
            "                      #  <child> := [ init, event, meter, label, wait, abort, complete ]\n"
            "                      #  <server-action> := [ adopt | delete ]\n"
            "                      #  <client-action> := [ fob | fail | block(default) ]\n"
            "                      #  <zombie-lifetime>:= lifetime of zombie in the server\n"
            "                      # example\n"
            "                      # add zombie :label:fob:0   # fob all child label request, & remove zombie as soon as possible\n"
            "\n"
            " delete variable name # if name is empty will delete -all- variables on the node\n"
            " delete time name     # To delete a specific time, enter the time in same format as show above,\n"
            "                      # or as specified in the defs file\n"
            "                      # an empty name will delete all time attributes on the node\n"
            " delete today name    # To delete a specific today attribute, enter in same format as show above,\n"
            "                      # or as specified in the defs file.\n"
            "                      # an empty name will delete all today attributes on the node\n"
            " delete date name     # To delete a specific date attribute, enter in same format as show above,\n"
            "                      # or as specified in the defs file\n"
            "                      # an empty name will delete all date attributes on the node\n"
            " delete day name      # To delete a specific day attribute, enter in same format as show above,\n"
            "                      # or as specified in the defs file\n"
            "                      # an empty name will delete all day attributes on the node\n"
            " delete cron name     # To delete a specific cron attribute, enter in same as specified in the defs file\n"
            "                      # an empty name will delete all cron attributes on the node\n"
            " delete event name    # To delete a specific event, enter name or number\n"
            "                      # an empty name will delete all events on the node\n"
            " delete meter name    # To delete a specific meter , enter the meter name\n"
            "                      # an empty name will delete all meter on the node \n"
            " delete label name    # To delete a specific label , enter the label name\n"
            "                      # an empty name will delete all labels on the node\n"
            " delete limit name    # To delete a specific limit , enter the limit name\n"
            "                      # an empty name will delete all limits on the node\n"
            " delete inlimit name  # To delete a specific inlimit , enter the inlimit name\n"
            "                      # an empty name will delete all inlimits on the node\n"
            " delete limit_path limit_name limit_path # To delete a specific limit path\n"
            " delete trigger       # A node can only have one trigger expression, hence the name is not required\n"
            " delete complete      # A node can only have one complete expression, hence the name is not required\n"
            " delete repeat        # A node can only have one repeat, hence the name is not required\n"
            "\n"
            " change variable name value    # Find the specified variable, and set the new value.\n"
            " change clock_type name        # The name must be one of 'hybrid' or 'real'.\n"
            " change clock_gain name        # The gain must be convertible to an integer.\n"
            " change clock_sync name        # Sync suite calendar with the computer.\n"
            " change event name(optional )  # if no name specified the event is set, otherwise name must be 'set' or 'clear'\n"
            " change meter name value       # The meter value must be convertible to an integer, and between meter min-max range.\n"
            " change label name value       # sets the label\n"
            " change trigger name           # The name must be expression. returns an error if the expression does not parse\n"
            " change complete name          # The name must be expression. returns an error if the expression does not parse\n"
            " change limit_max name value   # Sets the max value of the limit. The value must be convertible to an integer\n"
            " change limit_value name value # Sets the consumed tokens to value.The value must be convertible to an integer\n"
            " change repeat value           # If the repeat is a date, then the value must be a valid YMD ( ie. yyyymmdd)\n"
            "                               # and be convertible to an integer, additionally the value must be in range\n"
            "                               # of the repeat start and end dates. Like wise for repeat integer. For repeat\n"
            "                               # string and enum,  the name must either be an integer, that is a valid index or\n"
            "                               # if it is a string, it must correspond to one of enum's or strings list\n"
            "\nUsage::\n\n"
            "  try:\n"
            "     ci = Client()     # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "     ci.alter('/suite','change','trigger','b2 == complete')\n"
            "  except RuntimeError, e:\n"
            "     print str(e)\n"
            ;
}

const char* ClientDoc::force_state(){

   return
            "Force a node(s) to a given state\n\n"
            "When a :term:`task` is set to :term:`complete`, it may be automatically re-queued if it has\n"
            "multiple time :term:`dependencies`. In the specific case where a task has a single\n"
            "time dependency and we want to interactively set it to :term:`complete`\n"
            "a flag is set so that it is not automatically re-queued when set to complete.\n"
            "The flag is applied up the node hierarchy until reach a node with a :term:`repeat`\n"
            "or :term:`cron` attribute. This behaviour allow :term:`repeat` values to be incremented interactively.\n"
            "A :term:`repeat` attribute is incremented when all the child nodes are :term:`complete`\n"
            "in this case the child nodes are automatically re-queued\n::\n\n"
            "   void force_state(\n"
            "      string absolute_node_path: Path name to node. The path must begin with a leading '/'\n"
            "      State::State state       : [ unknown | complete | queued | submitted | active | aborted ]\n"
            "   )\n"
            "   void force_state(\n"
            "      list paths         : A list of absolute node paths. The paths must begin with a leading '/'\n"
            "      State::State state : [ unknown | complete | queued | submitted | active | aborted ]\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       # force a single node to complete\n"
            "       ci.force_state('/s1/f1',State.complete)\n"
            "\n"
            "       # force a list of nodes to complete\n"
            "       paths = [ '/s1/t1', '/s1/t2', '/s1/f1/t1' ]\n"
            "       ci.force_state(paths,State.complete)\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            "\nEffect::\n\n"
            "   Lets see the effect of forcing complete on the following defs::\n\n"
            "   suite s1\n"
            "      task t1; time 10:00             # will complete straight away\n"
            "      task t2; time 10:00 13:00 01:00 # will re-queue 3 times and complete on fourth \n\n"
            "In the last case (task t2) after each force complete, the next time slot is incremented.\n"
            "This can be seen by calling the Why command."

            ;
}

const char* ClientDoc::force_state_recursive(){

   return
            "Force node(s) to a given state recursively::\n\n"
            "   void force_state_recursive(\n"
            "      string absolute_node_path: Path name to node.The paths must begin with a leading '/'\n"
            "      State::State state       : [ unknown | complete | queued | submitted | active | aborted ]\n"
            "   )\n"
            "   void force_state_recursive(\n"
            "      list  paths         : A list of absolute node paths.The paths must begin with a leading '/'\n"
            "      State::State state  : [ unknown | complete | queued | submitted | active | aborted ]\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.force_state_recursive('/s1/f1',State.complete)\n"
            "\n"
            "       # recursively force a list of nodes to complete\n"
            "       paths = [ '/s1', '/s2', '/s1/f1/t1' ]\n"
            "       ci.force_state_recursive(paths,State.complete)\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::force_event(){

   return
            "Set or clear a :term:`event`::\n\n"
            "   void force_event(\n"
            "      string absolute_node_path:event: Path name to node: < event name | number>\n"
            "                                       The paths must begin with a leading '/'\n"
            "      string signal                  : [ set | clear ]\n"
            "   )\n"
            "   void force_event(\n"
            "      list paths    : A list of absolute node paths. Each path must include a event name\n"
            "                      The paths must begin with a leading '/'\n"
            "      string signal : [ set | clear ]\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.force_event('/s1/f1:event_name','set')\n"
            "\n"
            "       # Set or clear a event for a list of events\n"
            "       paths = [ '/s1/t1:ev1', '/s2/t2:ev2' ]\n"
            "       ci.force_event(paths,State.complete)\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::replace(){
   return
            "Replaces a :term:`node` in a :term:`suite definition` with the given path. The definition is in the :term:`ecflow_server`::\n\n"
            "   void replace(\n"
            "      string absolute_node_path: Path name to node in the client defs.\n"
            "                                 This is also the node we want to replace in the server.\n"
            "      string client_defs_file  : File path to defs files, that provides the definition of the new node\n"
            "      [(bool)parent=False]     : create parent families or suite as needed,\n"
            "                                 when absolute_node_path does not exist in the server\n"
            "      [(bool)force=False]      : check for zombies, if force = true, bypass checks\n"
            "   )\n"
            "\n"
            "   void replace(\n"
            "      string absolute_node_path: Path name to node in the client defs.\n"
            "                                 This is also the node we want to replace in the server.\n"
            "      Defs client_defs         : In memory client definition that provides the definition of the new node\n"
            "      [(bool)parent=False]     : create parent families or suite as needed,\n"
            "                                 when absolute_node_path does not exist in the server\n"
            "      [(bool)force=False]      : check for zombies, force = true, bypass checks\n"
            "   )\n"
            "\n"
            "Exceptions can be raised because:\n\n"
            "- The absolute_node_path does not exist in the provided definition\n"
            "- The provided client definition must be free of errors\n"
            "- If the third argument is not provided, then the absolute_node_path must exist in the server defs\n"
            "- replace will fail, if child task nodes are in :term:`active` / :term:`submitted` state\n\n"
            "After replace is done, we check trigger expressions. These are reported to standard output.\n"
            "It is up to the user to correct invalid trigger expressions, otherwise the tasks will *not* run.\n"
            "Please note, you can use check() to check trigger expression and limits in the server.\n\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.replace('/s1/f1','/tmp/defs.def')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            "\n"
            "   try:\n"
            "       ci.replace('/s1',client_defs) # replace suite 's1' in the server, with 's1' in the client_defs\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::kill(){
   return
            "Kills the job associated with the :term:`node`.::\n\n"
            "   void kill(\n"
            "      list paths: List of paths. Paths must begin with a leading '/' character\n"
            "   )\n"
            "   void kill(\n"
            "      string absolute_node_path: Path name to node to kill.\n"
            "   )\n"
            "\n"
            "If a :term:`family` or :term:`suite` is selected, will kill hierarchically.\n"
            "Kill uses the ECF_KILL_CMD variable. After :term:`variable substitution` it is invoked as a command.\n"
            "The ECF_KILL_CMD variable should be written in such a way that the output is written to %ECF_JOB%.kill, i.e::\n\n"
            "   kill -15 %ECF_RID% > %ECF_JOB%.kill 2>&1\n"
            "   /home/ma/emos/bin/ecfkill %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1\n\n"
            "\n"
            "Exceptions can be raised because:\n\n"
            "- The absolute_node_path does not exist in the server\n"
            "- ECF_KILL_CMD variable is not defined\n"
            "- :term:`variable substitution` fails\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.kill('/s1/f1')\n"
            "       time.sleep(2)\n"
            "       print ci.file('/s1/t1','kill')  # request kill output\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::status(){
   return
            "Shows the status of a job associated with a :term:`task` ::\n\n"
            "   void status(\n"
            "      list paths: List of paths. Paths must begin with a leading '/' character\n"
            "   )\n"
            "   void status(\n"
            "      string absolute_node_path\n"
            "   )\n\n"
            "If a :term:`family` or :term:`suite` is selected, will invoke status command hierarchically.\n"
            "Status uses the ECF_STATUS_CMD variable. After :term:`variable substitution` it is invoked as a command.\n"
            "The command should be written in such a way that the output is written to %ECF_JOB%.stat, i.e::\n\n"
            "   /home/ma/emos/bin/ecfstatus  %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1\n\n"
            "Exceptions can be raised because:\n\n"
            "- The absolute_node_path does not exist in the server\n"
            "- ECF_STATUS_CMD variable is not defined\n"
            "- :term:`variable substitution` fails\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.status('/s1/t1')\n"
            "       time.sleep(2)\n"
            "       print ci.file('/s1/t1','stats') # request status output\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::order(){
   return
            "Re-orders the :term:`node` s in the :term:`suite definition` held by the :term:`ecflow_server`\n\n"
            "It should be noted that in the absence of :term:`dependencies`,\n"
            "the order in which :term:`task` s are :term:`submitted`, depends on the order in the definition.\n"
            "This changes the order and hence affects the submission order::\n\n"
            "   void order(\n"
            "      string absolute_node_path: Path name to node.\n"
            "      string order_type        : Must be one of [ top | bottom | alpha | order | up | down ]\n"
            "   )\n"
            "   o top     raises the node within its parent, so that it is first\n"
            "   o bottom  lowers the node within its parent, so that it is last\n"
            "   o alpha   Arranges for all the peers of selected note to be sorted alphabetically\n"
            "   o order   Arranges for all the peers of selected note to be sorted in reverse alphabet\n"
            "   o up      Moves the selected node up one place amongst its peers\n"
            "   o down    Moves the selected node down one place amongst its peers\n\n"
            "Exceptions can be raised because:\n\n"
            "- The absolute_node_path does not exist in the server\n"
            "- The order_type is not the right type\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.order('/s1/f1','top')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::group(){
   return
            "Allows a series of commands to be executed in the :term:`ecflow_server`::\n\n"
            "   void group(\n"
            "       string cmds : a list of ';' separated commands \n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()               # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.group('get; show')\n"
            "       ci.group('get; show state') # show node states and trigger abstract syntax trees\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::begin_suite(){
   return
            "Begin playing the chosen :term:`suite` s in the :term:`ecflow_server`\n"
            "Note: using the force option may cause :term:`zombie` s::\n\n"
            "   void begin_suite\n"
            "      string suite_name     : begin playing the given suite\n"
            "      [(bool)force=False]   : bypass the checks\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                  # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.begin_suite('/suite1')      # begin playing suite '/suite1'\n"
            "       ci.begin_suite('/suite1',True) # begin playing suite '/suite1' bypass any checks"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::begin_all(){
   return
            "Begin playing all the :term:`suite` s in the :term:`ecflow_server`\n"
            "Note: using the force option may cause :term:`zombie` s::\n\n"
            "   void begin_all_suites(\n"
            "      [(bool)force=False] : bypass the checks\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()             # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.begin_all_suites()     # begin playing all the suites\n"
            "       ci.begin_all_suites(True) # begin playing all the suites, by passing checks\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::suspend(){
   return
            "Suspend :term:`job creation` / generation for the given :term:`node`::\n\n"
            "   void suspend(\n"
            "      list paths: List of paths. Paths must begin with a leading '/' character\n"
            "   )\n"
            "   void suspend(\n"
            "      string absolute_node_path: Path name to node to suspend.\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.suspend('/s1/f1/task1')\n"
            "       paths = ['/s1/f1/t1','/s2/f1/t2']\n"
            "       ci.suspend(paths)\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::resume(){
   return
            "Resume :term:`job creation` / generation for the given :term:`node`::\n\n"
            "   void resume(\n"
            "      list paths: List of paths. Paths must begin with a leading '/' character\n"
            "   )\n"
            "   void resume(\n"
            "      string absolute_node_path: Path name to node to resume.\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.resume('/s1/f1/task1')\n"
            "       paths = ['/s1/f1/t1','/s2/f1/t2']\n"
            "       ci.resume(paths)\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::job_gen(){
   return
            "Job submission for chosen Node *based* on :term:`dependencies`\n"
            "The :term:`ecflow_server` traverses the :term:`node` tree every 60 seconds, and if the dependencies are free\n"
            "does :term:`job creation` and submission. Sometimes the user may free time/date dependencies\n"
            "to avoid waiting for the server poll, this commands allows early job generation::\n\n"
            "   void job_generation(\n"
            "      string absolute_node_path: Path name for job generation to start from\n"
            "   )\n"
            "   If empty string specified generates for full definition.\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.job_generation('/s1')  # generate jobs for suite '/s1 \n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::delete_node(){
   return
            "Delete the :term:`node` (s) specified.\n\n"
            "If a node is :term:`submitted` or :term:`active`, then a Exception will be raised.\n"
            "To force the deletion at the expense of :term:`zombie` creation, then set\n"
            "the force parameter to true::\n\n"
            "   void delete(\n"
            "      list paths          : List of paths.\n"
            "      [(bool)force=False] : If true delete even if in 'active' or 'submitted' states\n"
            "                            Which risks creating zombies.\n"
            "   )\n"
            "   void delete(\n"
             "      string absolute_node_path: Path name of node to delete.\n"
            "      [(bool)force=False]       : If true delete even if in 'active' or 'submitted' states\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()                     # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.delete('/s1/f1/task1')\n"
            "\n"
            "       paths = ['/s1/f1/t1','/s2/f1/t2']\n"
            "       ci.delete(paths)                  # delete all tasks specified in the paths\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* ClientDoc::delete_all(){
   return
            "Delete all the :term:`node` s held in the :term:`ecflow_server`.\n\n"
            "The :term:`suite definition` in the server will be empty, after this call. **Use with care**\n"
            "If a node is :term:`submitted` or :term:`active`, then a Exception will be raised.\n"
            "To force the deletion at the expense of :term:`zombie` creation, then set\n"
            "the force parameter to true::\n\n"
            "   void delete_all(\n"
            "      [(bool)force=False] : If true delete even if in 'active' or 'submitted' states\n"
            "                            Which risks creating zombies.\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()    # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.delete_all()\n"
            "       ci.get_server_defs()\n"
            "   except RuntimeError, e:\n"
            "       print str(e);    # expect failure since all nodes deleted"
            ;
}

const char* ClientDoc::check()
{
   return
            "Check :term:`trigger` and :term:`complete expression` s and :term:`limit` s\n\n"
            "The :term:`ecflow_server` does not store :term:`extern` s. Hence all unresolved references\n"
            "are reported as errors.\n"
            "Returns a non empty string for any errors or warning::\n\n"
            "   string check(\n"
            "      list paths # List of paths.\n"
            "   )\n"
            "   string check(\n"
            "      string absolute_node_path\n"
            "   )\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()   # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       print ci.check('/suite1')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}
