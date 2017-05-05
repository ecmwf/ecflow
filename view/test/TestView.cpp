//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//============================================================================
#include <iostream>
#include <limits> // for std::numeric_limits<int>::max()
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>
#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "DurationTimer.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestView  )

static std::string ECFLOWVIEW_NAME = "ecflowview";

std::string find_ecflowview_path()
{
	std::string binDir;
	fs::path current_path = fs::current_path();
	if ( current_path.stem() == "Test"
	     || current_path.stem() == "view")
		binDir = "../view/bin/";
	else binDir = "view/bin/";

	// We have 3 variants debug,release,profile
#ifdef DEBUG
	return File::findPath( binDir, ECFLOWVIEW_NAME, "debug" );
#else
	std::string path = File::findPath( binDir, ECFLOWVIEW_NAME, "release" );
	if (path.empty()) {
		path = File::findPath( binDir, ECFLOWVIEW_NAME, "profile" );
	}
	return path;
#endif
}

void send_cmd(int fd, char* cmd) {
  std::cout << cmd;
  write(fd, cmd, strlen(cmd));
  ::sleep(2);
}

BOOST_AUTO_TEST_CASE( test_view )
{
   DurationTimer timer;
   cout << "View:: ...test_view\n"<< flush;

   std::string theViewInvokePath = find_ecflowview_path();
   if (theViewInvokePath.empty()) {
      std::cout << "ecflowview is not generated, test stops silently\n";
      return;
   }

   Defs theDefs; {
      suite_ptr suite = theDefs.add_suite( "test_view" ) ;
      suite->add_variable("SLEEPTIME","0");
      task_ptr task_a = suite->add_task("task_a");
      task_a->addMeter( Meter("meter",0,20,20) );
      task_a->addEvent( Event(1,"event") );
      task_a->addLabel( Label("task_a_label","Label1") );
   }

   // Start server if not already started, load the defs, and start playing
   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
			 ServerTestHarness::testDataDefsLocation("test_view.def"),
			 30, // timeout
			 false // do not wait for test completion
			 );

   // Ecflowview set up
   std::string HOME = getenv("HOME");
   std::string rcdir   = HOME + "/.ecflowrc_test";
   if ( !fs::exists( rcdir ) )  fs::create_directory(rcdir);

   std::string pipename = "/tmp/ecflowview_pipe";
   std::string servers = rcdir + "/servers";
   std::string options = rcdir + "/localhost.options";

   std::string cmd = "echo localhost localhost ${ECF_PORT:-3141} > " + servers;
   cmd += "\necho -e 'connect:false\nsuites: test_view\n' > " + options;
   cmd += "\nexport ECFLOWVIEW_INPUT=" + pipename + " ECFLOWRC=" + rcdir;
   cmd += "\nif [ ! -p $ECFLOWVIEW_INPUT ]; then rm -f $ECFLOWVIEW_INPUT; mknod $ECFLOWVIEW_INPUT p; fi;\n";
   cmd += theViewInvokePath;
   cmd += "&";
     
//   std::cout << cmd;
   if ( system( cmd.c_str() ) != 0)  assert(false); // failed
   ::sleep(2);
   int fd = open(pipename.c_str(), O_WRONLY);
   send_cmd(fd, (char*)"login localhost\n");
   send_cmd(fd, (char*)"logout localhost\n");
   send_cmd(fd, (char*)"quit\n");

   // remove generated data
   fs::remove(pipename.c_str());
   fs::remove_all(rcdir);
   fs::remove_all(TestFixture::smshome());
}

BOOST_AUTO_TEST_SUITE_END()
