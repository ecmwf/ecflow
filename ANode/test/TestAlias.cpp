/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Alias.hpp"
#include "Str.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_alias_create )
{
   cout << "ANode:: ...test_alias_create\n";
   std::string ecf_home = File::test_data("ANode/test/data/alias","ANode");

   task_ptr t ;
   Defs theDefs;
   {
      suite_ptr s = theDefs.add_suite("test_alias_create");
      family_ptr f = s->add_family("f");
      t = f->add_task("t");
      // ECFLOW-1278  set values for event,meter,labels, then ensure alias creation clears them
      Meter meter1("met",0,100,100); meter1.set_value(10);
      Meter meter2("met2",0,100,100); meter2.set_value(10);
      Event event1(1,"event1"); event1.set_value(true);
      Event event2(1,"event2"); event2.set_value(true);
      Label label1("name","value"); label1.set_new_value("new_value");
      Label label2("name2","value"); label2.set_new_value("new_value");
      t->addMeter(meter1);
      t->addMeter(meter2);
      t->addEvent(event1);
      t->addEvent(event2);
      t->addLabel(label1);
      t->addLabel(label2);
      s->add_variable(Str::ECF_HOME(),ecf_home);
   }

   // Create .usr file content
   std::vector<std::string> usr_file_vec;
   usr_file_vec.push_back("#This is a .usr file.");

   // Create list of variables
   NameValueVec used_variables;
   used_variables.push_back( std::make_pair(std::string("a"),std::string("a_value")));
   used_variables.push_back( std::make_pair(std::string("b"),std::string("b_value")));

   // Finally create the alias.
   alias_ptr alias = t->add_alias(usr_file_vec,used_variables);

   // Test that default node state is QUEUED
   BOOST_CHECK_MESSAGE(alias->state() == NState::QUEUED,"Expected initial state of QUEUED");

   // Test that CHILD specific attributes event, meter, labels are copied over from the task  AND that they are reset.
   BOOST_CHECK_MESSAGE(alias->meters().size() == 2,"Expected 2 meter to be copied from task but found " << alias->meters().size());
   BOOST_CHECK_MESSAGE(alias->events().size() == 2,"Expected 2 events to be copied from task but found " << alias->events().size());
   BOOST_CHECK_MESSAGE(alias->labels().size() == 2,"Expected 2 labels to be copied from task but found " << alias->labels().size());
   BOOST_FOREACH(const Meter& meter, alias->meters()){BOOST_CHECK_MESSAGE(meter.value() == meter.min(),"Expected meter value " << meter.min() << " but found " << meter.value() << " for " << meter.dump());}
   BOOST_FOREACH(const Event& event, alias->events()){BOOST_CHECK_MESSAGE(!event.value(),"Expected " << event.dump() << " to be clear");}
   BOOST_FOREACH(const Label& label, alias->labels()){BOOST_CHECK_MESSAGE(label.new_value().empty(),"Expected " << label.dump() << " to be clear");}


   // Ensure Task state was not changed
   BOOST_CHECK_MESSAGE(t->meters().size() == 2,"Did not expect task state to change, expected 2 meter but found: " << t->meters().size());
   BOOST_CHECK_MESSAGE(t->events().size() == 2,"Did not expect task state to change, expected 2 events but found:" << t->events().size());
   BOOST_CHECK_MESSAGE(t->labels().size() == 2,"Did not expect task state to change, expected 2 labels but found:" << t->labels().size());
   BOOST_FOREACH(const Meter& meter, t->meters()){BOOST_CHECK_MESSAGE(meter.value() == 10,"Expected meter value 10 but found " << meter.value() << " for " << meter.dump());}
   BOOST_FOREACH(const Event& event, t->events()){BOOST_CHECK_MESSAGE(event.value(),"Expected " << event.dump() << " to be set");}
   BOOST_FOREACH(const Label& label, t->labels()){BOOST_CHECK_MESSAGE(label.new_value() == "new_value","Expected label with 'new_value' but found " << label.new_value());}


   // test that user variables passed in got added as Variables
   BOOST_CHECK_MESSAGE(alias->variables().size() == 2,"Expected 2 variables to be create from input args but found " << alias->variables().size());

   // Test directory creation
   fs::path dir(ecf_home + "/" + t->absNodePath());
   BOOST_CHECK_MESSAGE(fs::exists(dir),"Expected directory to be created " + dir.string());

   // Test alias0.usr file creation
   fs::path usr_file(ecf_home + "/" + t->absNodePath() + "/" + "alias0.usr");
   BOOST_CHECK_MESSAGE(fs::exists(usr_file),"Expected alias0.usr file to be created " + usr_file.string());

   // Create another alias. This should get created as alias1.usr
   alias_ptr alias1 = t->add_alias(usr_file_vec,used_variables);
   fs::path usr_file1(ecf_home + "/" + t->absNodePath() + "/" + "alias1.usr");
   BOOST_CHECK_MESSAGE(fs::exists(usr_file1),"Expected alias1.usr file to be created " + usr_file1.string());

   // Test Defs::get_all_aliases()
   std::vector<alias_ptr> alias_vec;
   theDefs.get_all_aliases(alias_vec);
   BOOST_CHECK_MESSAGE(alias_vec.size() == 2,"Expected 2 aliases but found " << alias_vec.size());

   // Check alias remove
   BOOST_FOREACH(alias_ptr al,alias_vec) { al->remove();}
   alias_vec.clear();
   theDefs.get_all_aliases(alias_vec);
   BOOST_CHECK_MESSAGE(alias_vec.empty(),"Expected no aliases but found " << alias_vec.size());

   // Cleanup by removing the created directory
   fs::remove_all(ecf_home);
}

BOOST_AUTO_TEST_SUITE_END()
