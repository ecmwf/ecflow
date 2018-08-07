//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #11 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <stdio.h>
#include <strings.h>
#include "ecflowview.h"
#include "scripting.h"
#include "directory.h"
#include "input.h"
// cmd
#include "gui.h"
#include "node.h"
#include "host.h"
#include "selection.h"
#include "panel_window.h"
#include "uitop.h"
#include "menus.h"

// #include <sys/types.h>
// #include <sys/stat.h> 
#include <fcntl.h> 
#include "ecflowview.h"

int select_cmd(const char *host_name, const char *node_name)
{
  gui::raise();
  host::login(host_name);
  node* n = host::find(host_name,node_name);
  if(n) selection::notify_new_selection(n);
  return True;
}

int order_cmd(const char *path, const char *kind) 
{
  gui::raise();
  char *c = (char*) path; // host:/path
  const char* host_name = 0x0;
  const char* node_name = path;
  while (c) { 
    if (*c == ':') { *c = '\0'; node_name = ++c; } else {++c;}} 
  if (!host_name) host_name="localhost";
  host::login(host_name);
  node* xnode = host::find(host_name,node_name);
  if(xnode) { 
    selection::notify_new_selection(xnode);
    xnode->serv().command(clientName, "--order", node_name, kind, NULL);
  }
  return True;
}

int menu_cmd(const char *cmd)
{
  return script_menus(0, cmd);
}

int window_cmd(const char *name, int detached, int frozen)
{
  panel_window::new_window(selection::current_node(),name,detached,frozen);
  return True;
}

int login_cmd(const char *name)
{
  host::login(name);
  return True;
}

int logout_cmd(const char *name)
{
  host::logout(name);
  return True;
}

int quit_cmd()
{
  top_shell_c::quitCB(0x0, 0x0, 0x0);
  return True;
}

int process_command(const char *cmd) {
  if (!cmd) return False;

  if (!strncmp("select", cmd, 6)) {
      char host[80] = { 0, };
      char node[1024] = { 0, };
      sscanf(cmd, "select %s %s", host, node);
      if (host[0] != 0 && node[0] != 0) {
	std::cout << "#CMD (scripting): " << cmd << "\n";
	select_cmd(host, node);
      } else {
	std::cerr << "#CMD (scripting): err: " << cmd << "\n";
	return False;
      }

  } else if (!strncmp("order", cmd, 5)) {
      char kind[80] = { 0, };
      char node[1024] = { 0, };
      sscanf(cmd, "order %s %s", node, kind);
      if (kind[0] != 0 && node[0] != 0) {
	std::cout << "#CMD (scripting): " << cmd << "\n";
	order_cmd(node, kind);
      } else {
	std::cerr << "#CMD (scripting): err: " << cmd << "\n";
	return False;
      }

  } else if (!strncmp("menu", cmd, 4)) {
    menu_cmd(cmd);

  } else if (!strncmp("quit", cmd, 4)) {
    quit_cmd();

  } else if (!strncmp("login", cmd, 5)) {
      char host[80] = { 0, };
      sscanf(cmd, "login %s", host);
      if (host[0] != 0) {
	login_cmd(host);
      }

  } else if (!strncmp("output", cmd, 6)) {
      char node[2048] = { 0, };
      char path[2048] = { 0, };
      // sscanf(cmd, "output %s", node, path);
      if (node[0] != 0 && path[0] != 0) {
	// panel_window::new_window(selection::current_node(),"Output",detached,frozen);
      }
      // host::sfile(node, path);
  } else if (!strncmp("dir", cmd, 3)) {
      char node[2048] = { 0, };
      char path[2048] = { 0, };
      // sscanf(cmd, "dir %s %d", node, path);
      if (node[0] != 0 && path[0] != 0) {
	/* */
      }

  } else if (!strncmp("logout", cmd, 6)) {
      char host[80] = { 0, };
      sscanf(cmd, "logout %s", host);
      if (host[0] != 0) {
	logout_cmd(host);
      }

  } else if (!strncmp("window", cmd, 6)) {
      int detached = 0;
      int frozen   = 0;
      int len;
      char name[32] = { 0, };
      const char *ptr = cmd; 
      while ((sscanf(ptr, "%31[^ ]%n", name, &len) == 1)) {
	std::cerr << "#field: " << name << "\n";
	ptr += len;
	if (!strncmp("-d", name, 2)) detached = 1;
	if (!strncmp("-f", name, 2)) frozen   = 1;
	if (*ptr != ' ') { break; /* skip separator */ }
	ptr++;
	std::cout << "#CMD (scripting): process: " << name << "\n";
      }
      if (name[0] != 0) {
	std::cout << "#CMD (scripting): process: " << name << "\n";
	window_cmd(name, detached, frozen);
      } else {
	std::cerr << "#CMD (scripting): err: " << cmd << "\n";
	return False;
      }
  } else if (!strncmp("\n", cmd, 1)) {

  } else {
    std::cerr << "#CMD (scripting): ignored: " << cmd << "\n";
    return False;
  }
  
  std::cout << "#CMD (scripting): " << cmd << "\n";
  return True;
}

#undef SCRIPT_PYTHON
#ifdef SCRIPT_PYTHON
#ifdef linux
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include <boost/python.hpp> 
#include <boost/python/module.hpp> 
#include <boost/detail/lightweight_test.hpp>
using namespace boost::python;
namespace python = boost::python;

void runit(std::string const &script)
{
   python::dict global;
   // python::object result = 
   python::exec(oost::python::str(script), global, global);
   // BOOST_TEST(python::extract<int>(global["number"]) ==  42);
}

BOOST_PYTHON_MODULE(ecflowView)  
{
      def("window", window_cmd, args("name", "detached", "frozen"), "open a window detached/frozen");
      def("select", select_cmd, args("host", "node"), "select a node");
}

#endif
#endif

extern XtAppContext app_context;

class ecflowview_input {       
  std::string        name_;
  XtInputId          id_;
  int                fd_;
  std::string        line_;

public:

	ecflowview_input(const char* name):
		name_(name),
		fd_(-1)
	{
		open();
	}

	~ecflowview_input()
	{
		if(fd_ >= 0) XtRemoveInput(id_);
	}

	void done()
	{
		if(fd_ >= 0) XtRemoveInput(id_);
		::close(fd_);
		fd_ = -1;
		// We should check if its a pipe

		struct stat st;
		if(stat(name_.c_str(),&st) == 0)
		{
			if(S_ISFIFO(st.st_mode))
				open();
			else
				delete this;
		}
		else {
			perror(name_.c_str());
			delete this;
		}
	}

	void open()
	{
		fd_ = ::open(name_.c_str(),O_RDONLY|O_NONBLOCK); // |O_NDELAY);
		if(fd_ < 0) {
			perror(name_.c_str());
			delete this;
			return;
		}

		id_ = XtAppAddInput(app_context,fd_, XtPointer(XtInputReadMask),
			inputCB,this);
	}

	void input()
	{
		char c[2];
		if(::read(fd_,&c[0],1) != 1)
		{
			done();
		}
		if(c[0] == '\n')
		{
#ifdef SCRIPT_PYTHON
#ifdef linux
		  // Py_Initialize();
  init_module_ecflowView(); // thanks to boost macro
  bool error_expected = false;
  // Py_InitModule("ecflowView", ecflowViewMethods);
  std::cout << line_.c_str() << "\n";
  if ( 	// python::handle_exception(exec_test) ||
      python::handle_exception(boost::bind(runit, line_)))
	{
		if (PyErr_Occurred())
		{
			if (!error_expected) 
			  std::cerr << "Python Error detected";
			PyErr_Print();
		}
		else
		{
		  std::cerr << "A C++ exception was thrown  for which "
			    << "there was no exception translator registered.";
		}
	}
	// Boost.Python doesn't support Py_Finalize yet, so don't call it!
  line_ = "";
  return; // boost::report_errors();
  // PyRun_SimpleString(line_.c_str());
  // Py_Finalize(); // not supported by Boost
#endif
#else
		  process_command(line_.c_str());
		  line_ = "";
		  return;
#endif
		}
		else {
			c[1] = 0;
			line_ += c;
		}
	}

	static void inputCB(XtPointer data,int*,XtInputId*)
	{
		ecflowview_input* p = ((ecflowview_input*)data);
		p->input();
	}
};

scripting::scripting(const char* name):
	name_(name)
{
}

scripting::~scripting()
{
}

void scripting::init()
{
	char buf[1024];

	sprintf(buf,"%s/startup.script",directory::system());
	run(buf);

	sprintf(buf,"%s/startup.script",directory::user());
	run(buf);

	const char* file = getenv("ECFLOWVIEW_INPUT");
	if(file != 0) { new ecflowview_input(file);
	  std::cout << "# ecflowview listening: " << file << "\n";
	}
}

void scripting::run(const char* file)
{
	FILE* f = fopen(file,"r");
	if(!f) {
		//perror(file);
		return;
	}

	char line[1024];
	while(fgets(line,sizeof(line),f))
	{
		if(line[0]) line[strlen(line)-1] = 0;
		execute(line);
	}
	fclose(f);
}

void scripting::execute(const char* cmd)
{
#ifdef SCRIPT_PYTHON
#ifdef linux
  // Py_Initialize();
  init_module_ecflowView();
  // Py_InitModule("ecflowView", ecflowViewMethods);
  std::cout << cmd << "\n";
  python::handle_exception(boost::bind(runit, cmd));
  // PyRun_SimpleString(cmd);
  // Py_Finalize();
#endif
#else

  process_command(cmd);

#endif
}

scripting* scripting::find(const char* name)
{
	scripting* s = first();
	while(s)
	{
		if(strcmp(s->name_,name) == 0)
			return s;
		s = s->next();
	}
	return 0;
}

int scripting::dispatch(int argc,char **argv)
{
	scripting* s = find(argv[0]);
	if(s) return s->execute(argc,argv);
	fprintf(stderr,"cannot find command %s\n",argv[0]);
	return False;
}

IMP(scripting)
