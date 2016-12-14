//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #122 $ 
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

#include <stdarg.h>
#include <stdio.h>
#include <iostream> // shared_ptr
#include <memory>
#include <unistd.h>
#include <string>
#include <list>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "UrlCmd.hpp"
#include <time.h>

#include "ecflowview.h"
#include "ecflow.h"
#include "super_node.h"
#include <ostream>

#include "show.h"
#include "edit.h"

#ifndef scripting_H
#include "scripting.h"
#endif

#ifndef late_H
#include "late.h"
#endif

#ifndef zombie_H
#include "zombie.h"
#endif

#ifndef ecf_node_
#include "ecf_node.h"
#endif

#ifndef to_check_H
#include "to_check.h"
#endif

#ifndef directory_H
#include "directory.h"
#endif

#ifndef aborted_H
#include "aborted.h"
#endif

#ifndef restart_H
#include "restart.h"
#endif

#ifndef host_H
#include "host.h"
#endif

#ifndef passwrd_H
#include "passwrd.h"
#endif

#ifndef error_H
#include "error.h"
#endif

#ifndef confirm_H
#include "confirm.h"
#endif

#ifndef selection_H
#include "selection.h"
#endif

#ifndef init_H
#include "init.h"
#endif

#ifndef tree_H
#include "tree.h"
#endif

#ifndef gui_H
#include "gui.h"
#endif

#ifndef mail_H
#include "mail.h"
#endif

#ifndef configurator_H
#include "configurator.h"
#endif

#ifndef node_H
#include "node.h"
#endif

#ifndef logsvr_H
#include "logsvr.h"
#endif

#include "Version.hpp"

#include "panel_window.h"
#include <stdio.h>
#include <assert.h>
#include <boost/bind.hpp>

#include "menus.h"
/* #include <proc/readproc.h> */

using namespace std;

bool Updating::do_full_redraw_ = false;

class SelectNode {
public:
  SelectNode(const std::string& name)
   {
     node* n = selection::current_node();
     if (!n) return;
     if (name == n->serv().name()) {
       hostname_ = n->serv().name();
       current_ = selection::current_path();
     }
   }

   ~SelectNode()
   {
     if (hostname_.empty()) return;
     host* h = host::find(hostname_);
     if (h && !current_.empty()) {
       node *n = h->top()->find(current_);
       if (n) {
	  selection::notify_new_selection(n);
	}
     }  
   }

private:
  std::string current_, hostname_;
};

host::host( const std::string& name, const std::string& host, int number )
         : timeout(5)
#ifdef alpha
	 , configurable(name),
#endif
	 , observable()
	 , host_(host)
	 , number_(number)
	 , name_(name)
	 , connected_(false)
	 , after_command_(true)
	 , passwd_("-none-")
	 , timeout_(this, "timeout", 30)
	 , maximum_(this, "maximum", 60)
	 , drift_(this, "drift", true)
	 , connect_(this, "connect", false)
	 , suites_(this, "suites", std::vector<std::string>())
	 , aborted_(this, "aborted", true)
	 , restarted_(this, "restarted", true)
	 , late_(this, "late", true)
	 , poll_(this, "poll", true)
	 , direct_read_(this, "direct_read", true)
	 , new_suites_(this, "new_suites", true)
	 , zombie_(this, "zombie", false)
	 , aliases_(this, "aliases", false)
	 , late_family_(this, "late_family", false)
	 , to_check_(this, "to_check", false)
	 , chkmail_(true)
	 , top_(0)
	 , tree_(0)
	 , mail_(0)
	 , last_(0)
	 , history_len_(100)
	 , updating_(false)
	 , jobfile_length_(this, "jobfile_length", 10000)
{
   if (number < 1) return; // dummy server OK;

   if (number_) {
      tree_ = tree::new_tree(this);
      gui::add_host(name);
   }

   if (timeout_ < 30) timeout_ = 30;
   if (maximum_ < 30) maximum_ = 30;

   frequency(timeout_);
}

host::~host()
{
   if (tree_) {
      delete tree_;
   }
}

ehost::ehost( const std::string& name, const std::string& h, int number )
         : host(name, h, number)
{
   try {
      std::string port = boost::lexical_cast<std::string>(number);
      client_.set_host_port(host_.c_str(), port);
      client_.set_retry_connection_period(1);
   }
   catch ( std::exception& e ) {
      gui::message("# Exception caught in host::host ");
      gui::message(e.what());
   }
   if (connect_) login();
}

ehost::~ehost()
{
   connect_ = connected_;
   logout();
}

void host::remove_host( const std::string& name )
{
   gui::remove_host(name);
   ecf_nick_delete(name);
   ecf_nick_write();
   host* h = host::find(name);
   if (h) {
      h->logout();
      delete h;
   }
}

void host::broadcast( bool save )
{
   if (save) ecf_nick_write();
}

void host::login( const std::string& name )
{
   host* h = host::find(name);
   if (h) h->login();
}

void host::logout( const std::string& name )
{
   host* h = host::find(name);
   if (h) h->logout();
}

host* host::find( const std::string& machine, int number )
{
   host *h = extent < host > ::first();
   while ( h ) {
      if (h->host_ == machine && h->number_ == number) return h;
      h = h->extent < host > ::next();
   }
   return 0x0;
}

host* host::find( const std::string& name )
{
   host *h = extent < host > ::first();
   while ( h ) {
      if (h->name() && h->name() == name) return h;
      h = h->extent < host > ::next();
   }
   return 0;
}

node* host::find( const std::string& hostname, const std::string& n )
{
   host* h = find(hostname);
   if (h && h->top_) 
     return h->top_->find(n.c_str());

   return 0x0;
}

void host::status( Boolean force )
{
   host *h = extent < host > ::first();
   while ( h ) {
      if (force) h->reset(force);
      h->status();
      h = h->extent < host > ::next();
   }
}

void host::redraw_all()
{
   host *h = extent < host > ::first();
   while ( h ) {
      h->redraw();
      h = h->extent < host > ::next();
   }
}

void host::search( node_lister& s )
{
   if (top_) top_->search(s);
}

void host::logout()
{
   if (connected_) gui::logout(name());
   searchable::active(False);
   connected_ = false;

   if (tree_) {
      tree_->connected(False);
      tree_->xd_hide();
   }

   destroy_top(top_);
   top_ = 0x0;

   notify_observers();
}

void host::destroy_top(node* the_top) const
{
   if (the_top) {
      if (the_top->__node__()) {
         the_top->__node__()->nokids(); // Suites + all children
         the_top->__node__()->unlink();
         delete the_top->__node__(); // Defs
      }
      node::destroy(the_top);
   }
}

void ehost::logout()
{
   if (!connected_) return;

   try {  client_.ch1_drop(); }
   catch ( std::exception &e ) {
      gui::message("host::logout-error: %s", e.what());
   }

   host::logout();
}

void host::run()
{
   if (!poll_) return;
   update();
   if (drift_) drift(5, maximum_ * 60);
}

tmp_file host::file(node& n, std::string name)
{
   return tmp_file(NULL);
}

tmp_file host::edit( node& n, std::list<Variable>& l, Boolean preproc )
{
   return tmp_file(NULL);
}

tmp_file host::jobcheck( node& n, const std::string &cmd )
{
   return tmp_file(NULL);
}

tmp_file ehost::jobcheck( node& n, const std::string &cmd )
{
   std::string subcmd = n.variable(cmd);
   std::string job = n.variable("ECF_JOB");
   std::string stat = job + ".stat";
   if (n.__node__()) 
     if (n.__node__()->get_node()) 
       n.__node__()->get_node()->variableSubsitution(subcmd);
   std::string check = "sh " + subcmd;

   // tmp_file out(NULL);
   char *tmp = tmpnam("ecf_checkXXXX");
   command(check + " > " + tmp);
   return tmp_file(tmp);
   // return tmp_file(stat.c_str(), false);
}

tmp_file host::jobstatus( node& n, const std::string &cmd )
{
   return tmp_file(0x0);
}

tmp_file ehost::jobstatus( node& n, const std::string &cmd )
{
   command(clientName, "--status", n.full_name().c_str(), 0x0);
   return tmp_file(0x0);
}

bool host::zombies( int mode, const char* name )
{
   return false;
}

const std::vector<std::string>& ehost::messages( const node&n ) const
{
   try {
      client_.edit_history(n.full_name());
   }
   catch ( std::exception &e ) {
      gui::message("host::messages: %s", e.what());
   }
   return client_.server_reply().get_string_vec();
}

const std::vector<std::string>& host::messages( const node&n ) const
{
   static std::vector<std::string> vct;
   return vct;
}

bool host::get_zombies_list( std::vector<std::string>& list )
{
   return true;
}

bool ehost::get_zombies_list( std::vector<std::string>& list )
{
   gui::message("%s: fetching zombies_panel", this->name());
   try {
      client_.zombieGet();
   }
   catch ( std::exception &e ) {
      gui::message("host::zombies-error: %s", e.what());
      return false;
   }
   std::vector<Zombie> vect = client_.server_reply().zombies();

   if (vect.size() == 0) {
      gui::message("%s: No zombies at the moment", this->name());
      return false;
   }

   std::sort(
            vect.begin(),
            vect.end(),
            boost::bind(std::less<int>(), boost::bind(&Zombie::calls, _1),
                        boost::bind(&Zombie::calls, _2)));
   Zombie::pretty_print(vect, list);
   return true;
}

bool ehost::zombies( int mode, const char* name )
{

   if (!name) return false;

   try {
      gui::message("%s: updating zombies_panel", this->name());
      switch ( mode ) {
         case 1:
            client_.zombieFobCli(name);
            break;
         case 2:
            client_.zombieRemoveCli(name);
            break;
         case 3:
            client_.zombieFailCli(name);
            break;
         case 4:
            client_.zombieAdoptCli(name);
            break;
         case 5:
            client_.zombieKillCli(name);
            break;
            // case X: client_.zombieBlockCli(name); break; // ???
         default:
            break;
      }
   }
   catch ( std::exception &e ) {
      gui::message("host::zombies-error: %s", e.what());
      return false;
   }
   return true;
}

void host::set_loghost(node& n)
{
   loghost_ = n.variable("ECF_LOGHOST", true);
   logport_ = n.variable("ECF_LOGPORT");
   if (loghost_ == ecf_node::none()) {
      loghost_ = n.variable("LOGHOST", true);
      logport_ = n.variable("LOGPORT");
   }
   /* dynamic submission with load balacing may lead to 
      ECF_LOGHOST incorrect: let's fix that */
   std::string host =  n.variable("SCHOST", true);   
   std::string bkup   =  n.variable("SCHOST_BKUP", true);
   if (bkup == ecf_node::none()) { /* ECMWF specific below; a better way? */
     if (host == "cca")      bkup = "ccb";
     else if (host == "ccb") bkup = "cca";
   }
   if (bkup == ecf_node::none()) return; 

   std::string rid =  n.variable("ECF_RID");
   if (rid == ecf_node::none()) return; 

   bool use_altern = (rid.find(bkup) != std::string::npos);  
   if (!use_altern) return;

   size_t beg = loghost_.find(host);
   if (beg == std::string::npos) return; // not found
   loghost_ = n.variable("ECF_LOGHOST", true).replace(beg, host.length(), bkup);
   std::cout << "#MSG: using alternative loghost " + loghost_ << "\n";
}

void ehost::dir( node& n, const char* path, lister<ecf_dir>& l )
{
   set_loghost(n); 
   std::string::size_type pos = loghost_.find(n.variable("ECF_MICRO"));
   if (std::string::npos != pos) return;
   host::dir(n, path, l);
}

bool use_ecf_out_cmd(node&n, std::string path, ecf_dir *dir, std::string& content)
{
  /* used in conjonction with, for example:
     edit ECF_OUTPUT_CMD "/home/ma/map/bin/trimurti-out.sh -u %USER:0% -h %SCHOST:0% -j %ECF_JOB:0% -o %ECF_JOBOUT:0% -r %ECF_RID:0%" 
  */
  char buf[2048];
  std::string cmd = n.variable("ECF_OUTPUT_CMD", true);
  if (cmd == ecf_node::none()) return false;
  else if (cmd.length() < 3) return false; // may be empty space characters, ignore cmd
  else if (dir) cmd += " -d";
  else if (!path.empty()) cmd += " -f " + path;
  // return 0;

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) return false;

  while (!feof(pipe)) {
    if (fgets(buf, sizeof(buf), pipe) != NULL) {
      if (dir) { 
	dir->next = 0x0;
	char name[1200];
	sscanf(buf,"%d %d %d %d %d %d %d %s",
	       & dir->mode,
	       & dir->uid,
	       & dir->gid,
	       & dir->size,
	       & dir->atime,
	       & dir->mtime,
	       & dir->ctime,
	       name);
	
	dir->name_ = strdup(name);
	dir->next = new ecf_dir();
	dir = dir->next;

	XECFDEBUG std::cout << "#MSG:" << buf << " # " << name << "\n";
      } else { content += buf; }
    }
  }
  pclose(pipe);
  return true;
}

void host::dir( node& n, const char* path, lister<ecf_dir>& l )
{
   gui::message("%s: fetching file list", this->name());
   std::string content;
   std::auto_ptr<ecf_dir> dir(new ecf_dir());
   // if (use_ecf_out_cmd(n, path, dir.get(), content)) { l.scan(dir.get()); }   else 
   if (loghost_ != ecf_node::none()) {
      logsvr log_server(loghost_, logport_);

      if (log_server.ok()) {
	// std::auto_ptr<ecf_dir> 
	std::auto_ptr<ecf_dir> rdir(log_server.getdir(path));
	if (rdir.get()) {
	  l.scan(rdir.get());
         }
      }
   }

   if (path && direct_read_) {

      const char* p = path;
      const char* q = 0;

      while ( *p ) {
         if (*p == '/') q = p;
         p++;
      }

      if (q) {
         char basename[1024];
         char dirname[1024];
         strcpy(dirname, path);
         dirname[q - path] = 0;
         strcpy(basename, q + 1);

         char* c = basename;
         while ( *c ) {
            if (*c == '.') {
               if (*(c + 1)) {
                  *(c + 1) = 0;
                  break;
               } /* 201311 Pontus Request */
               else {
                  *c = 0;
               }
            }
            c++;
         }

         std::auto_ptr<ecf_dir> ldir (ecf_file_dir(dirname, basename, true));
         if (ldir.get()) {
	   l.scan(ldir.get());
         }
      }
   }
}

tmp_file host::script( node& n )
{
   return file(n, "ECF_SCRIPT");
}

tmp_file host::output( node& n )
{
   return file(n, "ECF_JOBOUT");
}

tmp_file host::job( node& n )
{
   return file(n, "ECF_JOB");
}

int host::status()
{
   int e = update();
   frequency(timeout_);
   return e;
}

void host::check_mail()
{
}

int ehost::command( const char* cmd, ... )
{
   int ac = 0;
   char *av[100], *s;
   va_list ap;

   va_start(ap, cmd);
   av[ac++] = strdup(cmd);
   while ( (s = va_arg(ap, char*)) )
      av[ac++] = strdup(s);
   va_end(ap);

   return command(ac, av);
}

int ehost::command( const std::string& str )
{
   const char *cmd = str.c_str();
   if (!cmd) return -1;

   if (str.substr(0, 3) == "sh ") {
      int pid = 0;
      if ((pid = fork()) == 0) /* the child */{
         execl("/bin/sh", "sh", "-c", str.substr(3).c_str(), NULL);
         _exit(127);
         return 0;
      }
      if (pid == -1)
         return 1;
      else
         return 0;
   }
   else if (str == "write menu") {
      menus::write();
      return 0;
   }

   int e = 0, ac = 0;
   char* av[100];
   char* line = strdup(cmd);

   char *c = (char*) "\"";
   char* s = strtok(line, c);
   if (s == 0)
      e = command(1, &line);
   else {
      c = (char*) "'";
      s = strtok(line, c);
      if (s == 0)
         e = command(1, &line);
      else {
         c = (char*) " ";
         s = strtok(line, c);
      }
   }
   do {
      av[ac++] = s;
      s = strtok(NULL, c);
   }
   while ( s != NULL );
   e = command(ac, av);

   if (line) free(line);
   return e;
}

int host::maxLines = 25000;

class init_hosts : public init {
   void run( int argc, char **argv )
   {
      scripting::init();
      host::init();
   }
};

void host::chat()
{
}

void host::check_all_mail()
{
   host *h = extent < host > ::first();
   while ( h ) {
      h->check_mail();
      h = h->extent < host > ::next();
   }
}

#include <fstream>

void mem_use( double& vmu, double& res )
{
   using std::ios_base;
   using std::ifstream;
   using std::string;
   unsigned long vsize;
   long rss;
   vmu = 0.0;
   res = 0.0;
   std::ifstream stat("/proc/self/stat", std::ios_base::in);
   std::string pid, comm, state, ppid, pgrp, session, tty_nr, tpgid, flags, 
     minflt, cminflt, majflt, cmajflt, utime, stime, cstime, priority, nice, 
     O, itrealvalue, starttime;
   stat >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid 
	>> flags >> minflt >> cminflt >> majflt >> cmajflt >> utime >> stime 
	>> cstime >> priority >> nice >> O >> itrealvalue >> starttime >> vsize 
	>> rss;
   stat.close();
   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
   vmu = vsize / 1024.0;
   res = rss * page_size_kb;
}

bool ehost::create_tree( int hh, int min, int sec )
{
   int then_sec = 0;
   XECFDEBUG {
      time_t now;
      time(&now);
      struct tm* then = gmtime(&now); // localtime(&now);
      then_sec = then->tm_sec;
      gui::message("%s: build %02d:%02d:%02d",
		   this->name(), then->tm_hour, then->tm_min, then->tm_sec);
      if (sec != then->tm_sec) {
         printf("# time get: %02d:%02d:%02d %s\n", hh, min, sec, this->name());
         printf("# time got: %02d:%02d:%02d %s\n", 
		then->tm_hour, then->tm_min, then->tm_sec, this->name());
      }
   }

   node *top = make_xnode<Defs>(client_.defs().get(), 0x0, *this);

   XECFDEBUG {
      time_t now;
      time(&now);
      struct tm* next = gmtime(&now); // localtime(&nnow);
      if (then_sec != next->tm_sec) 
	printf("# time blt: %02d:%02d:%02d %s\n", 
	       next->tm_hour, next->tm_min, next->tm_sec, this->name());
      gui::message("%s: built %02d:%02d:%02d", 
		   this->name(), next->tm_hour, next->tm_min, next->tm_sec);
   }

   if (!top) { 
     return false;
   }
   if (top_) {
      top->scan(top_);
      destroy_top(top_);
   }
   top_ = top;
   top_->active(poll_);
   notify_observers();
   top_->up_to_date();

   redraw();
   XECFDEBUG {
      double vmu, res;
      mem_use(vmu, res);
      if (top) { 
	int num = 0; node *n = top->kids(); 
	while (n) { num += 1; n = n->next(); } 
      }
      std::cout << "# usage: " << vmu << " " << res << "\n";
   }
   return true;
}

void ehost::reset( bool full, bool sync )
{
   if (!connected_ || !connect_) return;
   time_t now;
   time(&now);
   struct tm* curr = gmtime(&now); // localtime(&now);
   gui::message("%s: full tree %02d:%02d:%02d", 
		this->name(), curr->tm_hour, curr->tm_min, curr->tm_sec);
   SelectNode select(this->name());
   try {
      if (!tree_) tree_ = tree::new_tree(this);

      if (full) {
 	 XECFDEBUG std::cerr << "# reset full\n";
         const std::vector<std::string>& s = suites_;
         destroy_top(top_);
         top_ = 0x0;
         notify_observers();

         if (!s.empty()) {
            //   /* registering with empty set would lead
            //      to retrieve all server content,
            //      opposive of expected result */
            // } else
            // get all suite previously registered in GUI, and register
            // them with the server The associated handle is retained in
            // client_
            try { client_.ch1_drop(); }
            catch ( std::exception &e ) {
               std::cout << "# no drop possible: " << e.what() << "\n";
            }

            client_.reset(); // reset client handle + defs
            // This will add a new handle to client_
            client_.ch_register(new_suites_, s);
         }
      }
   }
   catch ( std::exception &e ) {
      XECFDEBUG std::cerr << "# reset exception " << e.what() << "\n";
      if (client_.defs().get()) {
         gui::error("host::reset-reg-error: %s", e.what());
      }
   }

   int hour = 0, min = 0, sec = 0;
   XECFDEBUG {
      time_t now;
      time(&now);
      struct tm* curr = gmtime(&now); // localtime(&now);
      hour = curr->tm_hour, min = curr->tm_min, sec = curr->tm_sec;
      gui::message("%s: start %02d:%02d:%02d", this->name(), hour, min, sec);
   }

   try {
      if (sync) client_.sync_local(); // this returns full defs
      searchable::active(False);
      create_tree(hour, min, sec);
      XECFDEBUG std::cerr << "# reset create tree\n";
   }
   catch ( std::exception &e ) {
     // XECFDEBUG 
      std::cerr << "# sync exception " << e.what() << "\n";
      gui::error("host::reset-sync-error: %s", e.what());
      const std::vector<std::string>& s = suites_;
      /* load one set
       then another
       checkpoint
       kill the server
       then restart the server
       + view update command */
      try {
         client_.reset();
         client_.ch_register(new_suites_, s);
      }
      catch ( std::exception &e ) {
         gui::error("host::reset-register-error: %s", e.what());
      }
   }
   searchable::active(True);
}

void host::aborted( node& n )
{
   if (n.type() == NODE_ALIAS) {
      if (aliases_) aborted::show(n);
   }
   else if (aborted_) aborted::show(n);
}

void host::restarted( node& n )
{
   if (n.type() == NODE_ALIAS) {
      if (aliases_) restart::show(n);
   }
   else if (restarted_) restart::show(n);
}

void host::late( node& n )
{
  if (late_family_ && n.type() == NODE_FAMILY)
    late::show(n);
  else if (late_) late::show(n);
}

void host::zombie( node& n )
{
   if (zombie_) zombie::show(n);
}

void host::to_check( node& n )
{
   if (to_check_) to_check::show(n);
}

void host::changed( resource& r )
{
   if (&r == &timeout_) frequency(timeout_);
}

void ehost::changed( resource& r )
{
   host::changed(r);
   if (&r == &poll_) {
      if (top_) top_->active(poll_);
      client_.set_host_port(machine(), boost::lexical_cast<std::string>(number()));
      connect_mngt(true);
      if (connected_ && poll_) status();
   }

   if ((&r == &new_suites_) && connected_) {
      // ch1_auto_add is used to control whether suites are
      // automatically added to handle. It should only be called if
      // suites had previously been registered
      try {
         if (client_.client_handle()) {
            client_.ch1_auto_add(new_suites_);
         }
         else {
            XECFDEBUG std::cerr << "# No suites previously registered ?";
         }
      }
      catch ( std::exception &e ) {
         gui::message("host::server-error: %s", e.what());
      }
   }
}

void host::redraw( bool create )
{
   if (create) {
     SelectNode select(this->name());

      if (top_) top_->unlink(true);
      create_tree(0, 0, 0);
   }
   else if (tree_) tree_->update_tree(true);
   if (top_) top_->reset();
}

str host::logfile() const
{
   char buf[1024];
   buf[0] = 0;
   return str(buf);
}

str ehost::logfile() const
{
   const char* home = top_ ? top_->variable("ECF_HOME").c_str() : 0;
   const char* log = top_ ? top_->variable("ECF_LOG").c_str() : 0;
   char buf[1024];
   buf[0] = 0;

   if (log) {
      if (log[0] != '/' && home)
         sprintf(buf, "%s/%s", home, log);
      else
         strcpy(buf, log);
   }
   return str(buf);
}

host& host::dummy()
{
   static host* h = new host("user.default", "user.default", 0);
   return *h;
}

void host::plug( node* from )
{
   do_plug(selection::current_node(), from);
}

void host::comp( node* from, const char* a, const char* b )
{
   do_comp(selection::current_node(), from, a, b);
}

int host::do_comp( node* into, node* from, const std::string& a, const std::string& b )
{
   if (!into || !from) return 0;
   std::stringstream out;
   out << "${COMPARE:=/home/ma/map/bin/compare.sh} " 
       << from->full_name() << ":";
   if (from->variable("ECF_NODE") != "(none)") {
      out << from->variable("ECF_NODE") << ":" 
	  << from->variable("ECF_PORT") << ":"

          << from->variable("ECF_LOGHOST", true) << ":" 
	  << from->variable("ECF_LOGPORT", true)

          << ":" << from->variable("ECF_JOBOUT", true) << " \t";
   }
   else {
      out << from->variable("SMSNODE") << ":" 
	  << from->variable("SMS_PROG") << ":"

          << from->variable("SMSLOGHOST", true) << ":" 
	  << from->variable("SMSLOGPORT", true) << ":"

          << from->variable("SMSJOBOUT", true) << " \t";
   }

   out << into->full_name() << ":";
   if (into->variable("ECF_NODE") != "(none)") {
      out << into->variable("ECF_NODE") << ":" 
	  << into->variable("ECF_PORT") << ":"
          << into->variable("ECF_LOGHOST", true) << ":" 
	  << into->variable("ECF_LOGPORT", true)
          << ":" << into->variable("ECF_JOBOUT", true) << " \t";
   }
   else {
      out << into->variable("SMSNODE") << ":" 
	  << into->variable("SMS_PROG") << ":"
          << into->variable("SMSLOGHOST", true) << ":" 
	  << into->variable("SMSLOGPORT", true) << ":"
          << into->variable("SMSJOBOUT", true) << " \t";
   }
   out << a << " \t" << b << "\n";
   const std::string cmd = out.str();
   std::cout << cmd;

   int pid = fork();
   if (pid == 0) { /* the child */
      execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
      _exit(127);
      return 0;
   }
   if (pid == -1) return 1;
   return 0;
}

int host::do_plug( node* into, node* from )
{
   if (!into || !from) return 1;

   str sf(from->full_name().c_str());
   str si(into->full_name().c_str());
   str sn = si + str("/") + str(from->name());

   host *destination = &into->serv();
   host *source = &from->serv();

   if (destination == source) {
      gui::error("# Node cannot be pluged to the same server");
      return 1;
   }

   if (!confirm::ask(false, "You are about pluging %s %s of %s into %s %s of %s.\n"
                     "Note that you will have to resume %s in the ECF %s.\n"
                     "Proceed?",
                     from->type_name(), sf.c_str(), source->name(), into->type_name(), si.c_str(),
                     destination->name(), sn.c_str(), destination->name())) return 1;

   str sp(from->parent()->full_name().c_str());
   if (sp != si) {
      if (!confirm::ask(false, "You are about to move the %s '%s' from a sub-tree named '%s' to\n"
                        "a sub-tree named '%s'. This may cause some problems,\n"
                        "specially if there are any active tasks. Do you want to proceed?",
                        from->type_name(), from->name().c_str(), sp.c_str(), si.c_str())) return 1;
   }

   if (destination->status()) {
      gui::error("# Cannot get status for %s. Pluging aborted.", destination->name());
      return 1;
   }
   if (source->status()) {
      gui::error("Cannot get status for %s. Pluging aborted.", source->name());
      return 1;
   }

   node *nfrom = source->top()->find(sf.c_str());
   std::string cmd;

   if (!(nfrom->status() == STATUS_SUSPENDED)) {
      if (source->command(clientName, "--suspend", nfrom->full_name().c_str(), 0x0)) {
         gui::error("Cannot suspend %s. Pluging aborted", sf.c_str());
         return 1;
      }
   }

   if (nfrom == 0) {
      cmd = "One of the node is gone after status.\n";
      cmd += "It must have been canceled. Pluging aborted.";
      gui::error(cmd.c_str());
      return 1;
   }

   gui::message("Pluging from %s to %s", source->name(), destination->name());
   cmd = "//";
   cmd += destination->machine();
   cmd += ":";
   cmd += (boost::lexical_cast<std::string>(destination->number())).c_str();
   cmd += si.c_str();
   if (source->command(clientName, "--plug", sf.c_str(), cmd.c_str(), 0x0)) {
      gui::error("Pluging aborted.");
      return 1;
   }

   source->status();
   destination->status();
   return 0;
}

struct dup_slash { // INT-74
  bool operator() (char x, char y) const {
    return x=='/' && y=='/';
  };
};

tmp_file ehost::sfile( node& n, std::string name )
{
   return host::sfile(n, name);
}

tmp_file host::sfile( node& n, std::string name )
{
   if (name == ecf_node::none()) return tmp_file((const char*) NULL);
   name.erase(std::unique(name.begin(), name.end(), dup_slash()), name.end()); // INT-74
   const char *cname = name.c_str();

   std::string::size_type pos = loghost_.find(n.variable("ECF_MICRO"));
   if (std::string::npos == pos && loghost_ != ecf_node::none()) {
      logsvr log_server(loghost_, logport_);
      if (log_server.ok()) {
 	 tmp_file tmp(log_server.getfile(name));
         if (access(tmp.c_str(), R_OK) == 0) return tmp;
      }
   }

   if ((access(cname, R_OK) == 0)) {
      return tmp_file(cname, false);
   }

   try {
      n.serv().command(clientName, "--file", "-n", cname, host::maxLines, 0x0);
   } 
   catch ( std::exception &e ) {
      gui::error("cannot get file from server: %s", e.what());
   }

   return tmp_file(cname, false);
}

const str& host::timefile()
{
   if (timefile_.c_str()[0] == 0)
      timefile_ = logfile();
   else if (!strncmp("(none)/(none)", timefile_.c_str(), 13)) timefile_ = logfile();
   return timefile_;
}

void host::hosts( host_lister& l )
{
   host *h = extent < host > ::first();
   while ( h ) {
      l.next(*h);
      h = h->extent < host > ::next();
   }
}

void host::change( const std::string& name, const std::string& machine, int number )
{
   bool logged = false;

   if (connected_) {
      logout();
      logged = true;
   }

   gui::rename_host(this->name(), name);

   ecf_nick_update(name, machine, number);

   name_ = name;
   host_ = machine;
   number_ = number;
   if (logged) login();
}

int host::origin() const
{
   return ecf_nick_origin(name());
}

static init_hosts init_hosts_instance;
std::map<int, host_maker*> host_maker::map_;
int host_maker::port_max = 65535;
static host_builder<ehost> ehost_builder_instance(0);

host* host::new_host( const std::string& p, const std::string& m, int n )
{
   ecf_nick_update(p, m, n);
   return host_maker::make_host(p, m, n);
}

IMP(host)

void host::login()
{
}

bool check_version( const std::string& server_version,  const std::string& viewer_version )
{
   // We know viewer version 4.1.0 is still compatible with old server versions 4.0.x
//   cout  << "server version '" << server_version << "'\n";
//   cout  << "viewer version '" << viewer_version << "'\n";
   if (viewer_version.find("4.1.") != std::string::npos && server_version.find("4.0.") != std::string::npos) {
      return true;
   }

   const char* v1 = server_version.c_str();
   const char* v2 = viewer_version.c_str();
   int num = 0;
   while ( v1 && v2 && num < 2 ) {
      if (*v1 == '.') num++;
      if (*v1 != *v2) return false;
      v1++;
      v2++;
   }
   return true;
}

void get_server_version( ClientInvoker& client, std::string& server_version )
{
   // ECF_ALLOW_NEW_CLIENT_OLD_SERVER allows each client ('client.allow_new_client_old_server')
   // to have its own archive version, hence FIRST:  go with what ever was set
   // See notes: ACore/src/boost_archive.hpp

   for(int av = ecf::boost_archive::version()-1; av >= ecf::boost_archive::version_1_47(); --av) {

      // First time in loop, go with what ever was set, including if client.allow_new_client_old_server() !=0
      try {
         client.server_version();
         server_version = client.server_reply().get_string();
         if (!server_version.empty()) return;
      }
      catch ( ... ) {}

      client.allow_new_client_old_server(av);
   }
}

void ehost::login()
{
   gui::message("Login to %s", this->name());
   host::logout();
   host::login();
   reset(true, true);

   client_.set_throw_on_error(true);
   try {
      client_.set_host_port(machine(), boost::lexical_cast<std::string>(number()));
      if (!connect_mngt(true)) {
         gui::message("%s: no reply", this->name());
         logout();
         connected_ = false;
         connect_ = false;
         return;
      }

      // if we can not get the server version, attempt backward compatibility
      std::string server_version;
      get_server_version(client_, server_version);
      if (server_version.empty()) {
         if (!confirm::ask( false, "%s (%s@%d): Could not connect\nTry again ?", 
			    this->name(), machine(), number())) {
             connect_ = false;
             connected_ = false;
             return;
          }
      }
      else {
	if (0)  // activate when an new release introduces loss of compat
         if (!check_version(server_version, ecf::Version::raw())) {
            if (!confirm::ask(
		     false,
                     "%s (%s@%d): version mismatch, server is %s, client is %s\ntry to connect anyway?",
                     this->name(), machine(), number(), server_version.c_str(),
                     ecf::Version::raw().c_str())) {
               connect_ = false;
               connected_ = false;
               return;
            }
         }
      }
      connect_ = true;
      connected_ = true;

      if (!tree_) tree_ = tree::new_tree(this);
      reset(true); // done later with update (test empty server)

      enable();
      if (tree_ != 0x0) {
         tree_->xd_show();
         tree_->connected(True);
         redraw();
      }
      gui::login(name());
      searchable::active(True);
   }
   catch ( std::exception& e ) {
      searchable::active(False);
      gui::error("Login to %s failed (%s)", this->name(), e.what());
      if (!tree_) return;
      if (connected_) {
         tree_->update_tree(false);
      }
      else {
         tree_->connected(False);
         if (!top_) 
	   top_ = make_xnode<Defs>(0x0, 0, *this);
      }
   }

   update();
}

tmp_file ehost::file(node& n, std::string name)
{
  std::string error;
  bool read = direct_read_;
  std::string no_script = n.variable("ECF_NO_SCRIPT");
  name.erase(std::unique(name.begin(), name.end(), dup_slash()), name.end()); // INT-74

  if (name == "ECF_SCRIPT") {
    error = "no script!\n"
      "check ECF_FILES or ECF_HOME directories, for read access\n"
      "check for file presence and read access below files directory\n"
      "or this may be a 'dummy' task.\n";    
  } else if (name == "ECF_JOB") {
    std::string filename = n.variable(name);
    if (read && (access(filename.c_str(), R_OK) == 0))
      return tmp_file(filename.c_str(), false);
    else if (!no_script.empty()) {
      // pass
      // error = "ECF_NO_SCRIPT! no script to be found, look at ECF_JOB_CMD";
    } else if (std::string::npos != filename.find(".job0")) {
	error = "job0: no job to be generated yet!";
	return tmp_file(error);
    } else 
      error = "no script!\n"
	"check ECF_HOME,directory for read/write access\n"
	"check for file presence and read access below\n"
	"The file may have been deleted\n"
	"or this may be a 'dummy' task.\n";    

  } else if (0) { // boost::algorithm::ends_with(name, ".0")) {
    error = "no output to be expected when TRYNO is 0!\n";
    return tmp_file(error);

  } else if (name != ecf_node::none()) { // Try logserver
    if (n.isCmdFailed()) {
      error = "Submission command Failed! check .sub file, ssh, or queueing system reported error";
      return tmp_file(error);
      
    }
      std::string::size_type pos = loghost_.find(n.variable("ECF_MICRO"));
      std::string content;
      if (use_ecf_out_cmd(n, name, NULL, content)) {
	tmp_file tmp(content);
	return tmp;
      } else if (std::string::npos == pos && loghost_ != ecf_node::none()) {
         logsvr log_server(loghost_, logport_);
         if (log_server.ok()) {
            tmp_file tmp = log_server.getfile(name); // allow more than latest output
            if (access(tmp.c_str(), R_OK) == 0) return tmp;
         }
      }
   }
   if (read && (access(name.c_str(), R_OK) == 0)) {
      return tmp_file(name.c_str(), false);
   } else {
      gui::message("%s: fetching %s", this->name(), name.c_str());
      try {
         if (name == "ECF_SCRIPT")
            client_.file(n.full_name(), "script");
         else if (name == "ECF_JOB") {
	   client_.file(n.full_name(), "job", 
			boost::lexical_cast<std::string>(jobfile_length_));
	 }
         else // if (name == "ECF_JOBOUT")
            client_.file(n.full_name(), "jobout");
         //else {
         //   client_.file(n.full_name(), "jobout");
         //}

         // Do *not* assign 'client_.server_reply().get_string()' to a separate string, since
         // in the case of job output the string could be several megabytes.
         return tmp_file( client_.server_reply().get_string()
			  + "\n# file is served by ecflow-server\n" );
      } 
      catch ( std::exception &e ) {
 	std::cerr << "host::file-error:" << e.what() << "\n";
         gui::message("host::file-error: %s", e.what());
      }
   }
   if (!no_script.empty())
     error = "ECF_NO_SCRIPT! no script to be found, look at ECF_JOB_CMD";
   return tmp_file(error);
}

tmp_file ehost::edit( node& n, std::list<Variable>& l, Boolean preproc )
{  
   std::string no_script = n.variable("ECF_NO_SCRIPT");
   gui::message("%s: fetching source", this->name());
   try {
      if (preproc)
         client_.edit_script_preprocess(n.full_name());
      else
         client_.edit_script_edit(n.full_name());
      return tmp_file(client_.server_reply().get_string());
   } 
   catch ( std::exception &e ) {
       gui::error("host::edit-error: %s", e.what());
   } 
   catch ( ... ) {
       gui::error("host::edit-error");
   }
  std::string error = "no script!\n"
"\n"
"check server->History:\n"
"\tsome suite variable may be 'unterminated' (micro character missing) in script or include files\n"
"\tcheck duplicate occurences of micro character when it is expected in the job (%% becomes %)\n"
"\tuse %nopp ... %end or %includenopp <file.h> to disable job preprocessing where needed\n"
"\tan include file may not be found\n"
"check ECF_FILE directory is accessible, by opening the Script panel\n"
"check ECF_INCLUDE directory is accessible from the server\n"
"\tit must contain the included files (or links)\n"
"client must be capable to create temporary file:\n"
"\tcheck /tmp directory with write access, and space available,\n"
"or preprocessed file may be truncated beyond some size.\n";
   if (!no_script.empty()) {
     std::string error = "ECF_NO_SCRIPT! no script to be found, look at ECF_JOB_CMD"
       + no_script.empty();
     // return tmp_file(error);
   }

  return tmp_file(error);
}

tmp_file host::manual( node& n )
{
   std::string man = "no manual...";
   return tmp_file(man);
}

tmp_file ehost::manual( node& n )
{
   gui::message("%s: fetching manual", this->name());
   try {
      client_.file(n.full_name(), "manual");
      if (client_.server_reply().get_string().empty()) {
         std::string man = "no manual...";
         return tmp_file(man);
      }
      return tmp_file(client_.server_reply().get_string());
   }
   catch ( std::exception &e ) {
      gui::message("host::manual-error: %s", e.what());
   }

   std::string man = "no manual...";
   return tmp_file(man);
}

void host::send( node& n, Boolean alias, Boolean run, NameValueVec& v, 
		 const char* file )
{
}

void ehost::send( node& n, Boolean alias, Boolean run, NameValueVec& v, 
		  const char* file )
{
   std::vector<std::string> content;
   char line[4096];
   FILE *f = fopen(file, "r");
   if (!f) {
      gui::syserr(file);
      return;
   }
   while ( fgets(line, sizeof(line), f) ) {
      line[strlen(line) - 1] = 0;
      content.push_back(line);
   }
   gui::message("%s: sending script_panel", this->name());

   try {
      client_.edit_script_submit(n.full_name(), v, content, alias, run);
   }
   catch ( std::exception &e ) {
      gui::error("host::send-error: %s", e.what());
   }
   status();
   fclose(f);
}

const std::vector<std::string>& host::suites() const
{
   return suites_;
}

void ehost::suites( int which, std::vector<std::string>& l )
/* Menu Suites..., or Suites tab */
{
   try {
      switch ( which ) {
         case SUITES_LIST:
            client_.suites();
            l = client_.server_reply().get_string_vec();
            break;
         case SUITES_MINE:
            l = suites_;
            break;
         case SUITES_REG:
            gui::message("%s: registering to suites", this->name());
            suites_ = l;
            try {
               if (l.empty()) {
                  try { client_.ch1_drop(); }
                  catch ( std::exception &e ) {
                     std::cout << "# no drop possible: " << e.what() << "\n";
                  }

                  // reset handle to zero , and clear the defs
                  client_.reset();
               }
               client_.ch_register(new_suites_, suites_);
               status();
               redraw();
            }
            catch ( std::exception &e ) {
               gui::error("host::suites-reg-error: %s", e.what());
            }
            break;
         default:
            gui::message("%s: suites, what?");
            break;
      }
   }
   catch ( std::exception &e ) {
      if (client_.defs().get()) { /* ignore empty server */
         gui::error("host::suites-error: %s", e.what());
      }
   }
}

void host::suites( node* n, bool one )
/* register only one suite with menu hide-other-suites (right-mouse-button,
 on the server area, close to server node, not on the node itself */
{
   while ( n ) {
      if (n->type() == NODE_SUITE) {
         static std::vector<std::string> l;
         if (!one) l = suites_;
         l.push_back(n->name());
         suites(SUITES_REG, l);
         break;
      }
      n = n->parent();
   }
}

int host::update()
{
   return TRUE;
}
extern XtAppContext app_context;

void ehost::update_reg_suites( bool get_ch_suites )
{
   if (new_suites_) { // SUP-398 // temporary add higher load on the server
      if (get_ch_suites) {
         try {
            client_.ch_suites();
         }
         catch ( std::exception& e ) {
            gui::message("host::update-reg-suite-error: %s", e.what());
         }
         const std::vector<std::pair<unsigned int, std::vector<std::string> > >& vct = client_.server_reply().get_client_handle_suites();
         for(size_t i = 0; i < vct.size(); ++i) {
            if (vct[i].first == (unsigned int) client_.client_handle()) {
               suites_ = vct[i].second;
               break;
            }
         }
      }
      else {
         const std::vector<suite_ptr>& suites_vec = client_.defs()->suiteVec();
         std::vector<std::string> suites;
         suites.reserve(suites_vec.size());
         for(size_t i = 0; i < suites_vec.size(); ++i) {
            suites.push_back(suites_vec[i]->name());
         }
         suites_ = suites;
      }
   }
}

int ehost::update()
{
   int err = -1;
   if (!connected_) return err;

   SelectNode select(this->name());
   if (updating_) return 0; // SUP-423
   Updating update(this);   // SUP-423
   gui::watch(True);
   last_ = ::time(0);

   try {
      if (app_context) XtAppAddTimeOut(app_context, 20 * 1000, NULL, NULL);

      time_t now;
      time(&now);
      struct tm* curr = gmtime(&now); // localtime(&now);
      gui::message("%s: checking status %02d:%02d:%02d",
		   this->name(), curr->tm_hour, curr->tm_min, curr->tm_sec);
      client_.news_local(); // call the server
      if (tree_) tree_->connected(True);

      XECFDEBUG {
         time_t now;
         time(&now);
         struct tm* next = gmtime(&now); // localtime(&now);
         if (curr->tm_sec != next->tm_sec) {
            printf("# time chk: %02d:%02d:%02d %s\n", 
		   curr->tm_hour, curr->tm_min, curr->tm_sec, this->name());
            printf("# time nws: %02d:%02d:%02d %s\n", 
		   next->tm_hour, next->tm_min, next->tm_sec, this->name());
         }
      }
      switch ( client_.server_reply().get_news() ) {
         case ServerReply::NO_NEWS:
            gui::message("::nonews\n");
            if (top_) top_->up_to_date();
            return 0;
            break;
         case ServerReply::DO_FULL_SYNC: // 4 calls to the server:
            /* ch_suites + drop + reg_suites + sync_local */
            gui::message("::fullsync\n");
            if (top_) top_->up_to_date();
            update_reg_suites(true);
            reset(true);
            return 0;
            break;
         case ServerReply::NO_DEFS:
            reset(true);
            return 0;
            break;
         case ServerReply::NEWS:
            // there were some kind of changes in the server
            // request the changes from the server & sync with
            // defs on client_

            client_.sync_local();
            // full_sync==true:  no notification on the GUI side

            // full_sync==false: incremental change, notification
            // received through ::update (ecf_node)

            gui::message("%s: receiving status", this->name());

            if (client_.server_reply().full_sync()) {
               update_reg_suites(false); // new suite may have been added
               reset(false, false); // SUP-398
            }
            else {
               gui::message("%s: updating status", this->name());
               XECFDEBUG std::cout << "# " << name() << ": small update\n";

               if (Updating::full_redraw()) {
                  redraw(true);
               }
               else if (tree_) tree_->update_tree(false); 
	       // fp:60043 Issue with Ecflow updating on console VM
               // redraw(false); // too much blinking with this
            }
            err = 0;
            break;
         default:
            break;
      }
   }
   catch ( std::exception& e ) {
      if (tree_ != 0x0) tree_->connected(False);
      err = -1;
      gui::message("host::news-error: %s", e.what());
      XECFDEBUG std::cerr << "# host::news-error: " << e.what() << "\n";
   }
   return err;
}

int ehost::command( int argc, char **argv )
{
   int result = -1;

   if (argc < 1) return FALSE;

   if (!strcmp(argv[1], "--enable_logging")) {
      client_.enable_logging("ecflow_client.log");
      return true;
   }
   if (!strcmp(argv[1], "--disable_logging")) {
      client_.disable_logging();
      return true;
   }
   else if (!strcmp(argv[1], "--url")) {
      if (argc == 3) {
         UrlCmd urlCmd(client_.defs(), argv[2]);
         try {
            urlCmd.execute();
         }
         catch ( ... ) {
            gui::error("cannot-open-url or substitution-error\n%s", argv[2]);
         }
         return true;
      }
   }

   gui::message("command issued ...");
   if (!strcmp(argv[0], clientName)) {
      try {
         int i = 0;
         std::cout << "# CMD: ";
         while ( i < argc )
            std::cout << argv[i++] << " ";
         result = client_.invoke(argc, argv);
         std::cout << "--port " << number() << " --host " << machine() << " # ack\n";
      }
      catch ( std::exception &e ) {
         gui::error("host::command-error:\n%s\n", e.what());
      }
   }
   else {
      int pid = fork();
      if (pid == 0) { /* the child */
         execl("/bin/sh", "sh", "-c", argv, NULL);
         _exit(127);
         return 0;
      }
      if (pid == -1) return 1;
   }
   if (after_command_) status();

   return result;
}

std::list<std::string>& host::history( std::string& last )
{
   return hist_;
}

std::list<std::string>& ehost::history( std::string& last )
{
   gui::message("%s: fetching history", this->name());
   try {
      client_.getLog(history_len_);
      boost::split(hist_, client_.server_reply().get_string(), 
		   boost::is_any_of("\n"));
   }
   catch ( std::exception& e ) {
      gui::message("history failed: ", e.what());
   }
   return hist_;
}

bool host::connect_mngt( bool connect )
{
   return true;
}

bool ehost::connect_mngt( bool connect )
{
   if (!connect) return true;
   if (!connect_) return true;
   bool rc = true;
   try {
      gui::message("%s: ping", this->name());
      client_.pingServer();

      if (connect) {
         rc = true;
         connected_ = true;
      }
      else {
         connected_ = false;
         rc = false;
      }
   }
   catch ( std::exception &e ) {
      connected_ = false;
      rc = false;
      gui::message("# Exception caught in ehost::connect_mngt");
      gui::message(e.what());
   }

   if (tree_) tree_->connected(rc);
   if (!rc) gui::logout(name());
   return rc;
}

const std::string host::reply() const
{
   return "";
}

const std::string ehost::reply() const
{
   return client_.server_reply().get_string();
}

void host::init()
{
   ecf_nick_load();
}

host* host_maker::make_host( std::string name, std::string machine, int port )
{
   std::map<int, host_maker*>::const_iterator it = map_.begin();
   host * out = 0x0;
   if (port < host_maker::port_max) {
      it = map_.find(0);
      if (it != map_.end())
         out = it->second->make(name, machine, port);
      else XECFDEBUG std::cerr << "# cannot create ehost\n";
   }
   else {
      it = map_.find(1);
      if (it != map_.end())
         out = it->second->make(name, machine, port);
      else XECFDEBUG std::cerr << "# cannot create shost " << name << "\t" << machine << "\t"
                               << port << "\n";
   }

   return out;
}

void host::login( const std::string& name, int num )
{
   host *h = NULL;
   if (num) h = host::find(name, num);
   if (!h) h = host::find(name, ECF_PROG);
   if (!h) h = host::find(name, SMS_PROG);
   if (!h) h = host_maker::make_host(name, name, num);
   if (h) h->login();
}

void ehost::stats( std::ostream& buf )
{
   gui::message("%s: fetching stats", this->name());
   try {
      client_.stats();
      client_.server_reply().stats().show(buf);
   }
   catch ( std::exception& e ) {
   }
}
