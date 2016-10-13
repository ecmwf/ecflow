//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #16 $ 
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
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "MyDefsFixture.hpp"
#include "TestHelper.hpp"
#include "DefsStructureParser.hpp"
#include "DurationTimer.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_delete_node_cmd )
{
	cout << "Base:: ...test_delete_node_cmd\n";

	MyDefsFixture fixtureDef;
	MockServer mockServer(&fixtureDef.defsfile_);

	// IMPORTANT: ******************************************************************************
	// If the PathsCmd is given a EMPTY list of paths, it will delete *EVERYTHING*
	// *****************************************************************************************

   // Delete all Aliases
   {
      std::vector<alias_ptr> vec;
      fixtureDef.defsfile_.get_all_aliases(vec);
      BOOST_CHECK_MESSAGE( vec.size() > 0,"Expected > 0 aliases but found " << vec.size());

      std::vector<std::string> paths; paths.reserve(vec.size());
      BOOST_FOREACH(alias_ptr t, vec) { paths.push_back(t->absNodePath()); }

      size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
      BOOST_CHECK_MESSAGE( !paths.empty(),"Expected paths to be specified, *OTHERWISE* we delete all nodes");
      PathsCmd cmd(PathsCmd::DELETE,paths);
      cmd.setup_user_authentification(UserCmd::get_user(),string());
      STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
      BOOST_CHECK_MESSAGE(returnCmd->ok(),"Failed to delete aliases");
      {
         // ECFLOW-434
         const std::deque<std::string>& edit_history = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
         size_t edit_history_size_after = edit_history_size_before + paths.size();
         if ( edit_history_size_after > Defs::max_edit_history_size_per_node())
            edit_history_size_after = Defs::max_edit_history_size_per_node();
         BOOST_CHECK_MESSAGE( edit_history.size() == edit_history_size_after, "Expected   " << edit_history_size_after << " edit history but found " << edit_history.size());
      }

      std::vector<alias_ptr> afterDeleteVec;
      fixtureDef.defsfile_.get_all_aliases(afterDeleteVec);
      BOOST_REQUIRE_MESSAGE( afterDeleteVec.empty(),"Expected all aliases to be deleted but found " << afterDeleteVec.size());
   }

	// Delete all tasks
	{
		std::vector<Task*> vec;
		fixtureDef.defsfile_.getAllTasks(vec);
		BOOST_CHECK_MESSAGE( vec.size() > 0,"Expected > 0 tasks but found " << vec.size());

	   std::vector<std::string> paths; paths.reserve(vec.size());
	   BOOST_FOREACH(Task* t, vec) { paths.push_back(t->absNodePath()); }

      BOOST_CHECK_MESSAGE( !paths.empty(),"Expected paths to be specified, *OTHERWISE* we delete all nodes");
      size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
      PathsCmd cmd(PathsCmd::DELETE,paths);
	   cmd.setup_user_authentification(UserCmd::get_user(),string());
	   STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
	   BOOST_CHECK_MESSAGE(returnCmd->ok(),"Failed to delete tasks");
      {
         const std::deque<std::string>& edit_history = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
         size_t edit_history_size_after = edit_history_size_before + paths.size();
         if ( edit_history_size_after > Defs::max_edit_history_size_per_node())
            edit_history_size_after = Defs::max_edit_history_size_per_node();
         BOOST_CHECK_MESSAGE( edit_history.size() == edit_history_size_after, "Expected   " << edit_history_size_after << " edit history but found " << edit_history.size());
      }


		std::vector<Task*> afterDeleteVec;
		fixtureDef.defsfile_.getAllTasks(afterDeleteVec);
		BOOST_REQUIRE_MESSAGE( afterDeleteVec.empty(),"Expected all tasks to be deleted but found " << afterDeleteVec.size());
	}

	// Delete all Families
	{
		// DONT use getAllFamilies as this will return hierarchical families
		// we don't want to delete families twice
		//		std::vector<Family*> vec;
		//		fixtureDef.defsfile_.getAllFamilies(vec);
 	}

	// Delete all Suites
	{
		std::vector<suite_ptr> vec =  fixtureDef.defsfile_.suiteVec();
		BOOST_CHECK_MESSAGE( vec.size() > 0,"Expected > 0 Suites but found " << vec.size());
		BOOST_FOREACH(suite_ptr s, vec) {
		   {
		      // Delete all Families
            // *********************************************************************************************
            // **EXPLICITLY** check for empty paths otherwise we can end up deleting ALL suites accidentally
            // if PathsCmd is given an empty list, we will delete all nodes including the suites
            // *********************************************************************************************
		      std::vector<family_ptr> familyVec = s->familyVec();
		      // 	DONT USE:
		      //        BOOST_FOREACH(family_ptr f, s->familyVec()) {
		      //  As with this will invalidate the iterators.
		      std::vector<std::string> paths; paths.reserve(vec.size());
		      BOOST_FOREACH(family_ptr f, familyVec) { paths.push_back(f->absNodePath()); }

		      if (!paths.empty()) {
		         size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
		         PathsCmd cmd(PathsCmd::DELETE,paths);
		         cmd.setup_user_authentification(UserCmd::get_user(),string());
		         STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
		         BOOST_CHECK_MESSAGE(returnCmd->ok(),"Failed to delete families");
		         BOOST_REQUIRE_MESSAGE( s->familyVec().empty(),"Expected all Families to be deleted but found " << s->familyVec().size());
		         {
		            const std::deque<std::string>& edit_history = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
	               size_t edit_history_size_after = edit_history_size_before + paths.size();
	               if ( edit_history_size_after > Defs::max_edit_history_size_per_node())
	                  edit_history_size_after = Defs::max_edit_history_size_per_node();
		            BOOST_CHECK_MESSAGE( edit_history.size() == edit_history_size_after, "Expected   " << edit_history_size_after << " edit history but found " << edit_history.size());
		         }
		      }
		   }

			// delete the suite
         size_t edit_history_size_before = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH()).size();
			std::string absNodePath = s->absNodePath();
	      PathsCmd cmd(PathsCmd::DELETE,absNodePath);
			cmd.setup_user_authentification(UserCmd::get_user(),string());
			STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
			BOOST_CHECK_MESSAGE(returnCmd->ok(),"Failed to delete suite at path " << absNodePath);
	      {
            const std::deque<std::string>& edit_history = fixtureDef.defsfile_.get_edit_history(Str::ROOT_PATH());
            size_t edit_history_size_after = edit_history_size_before + 1;
            if ( edit_history_size_after > Defs::max_edit_history_size_per_node())
               edit_history_size_after = Defs::max_edit_history_size_per_node();
            BOOST_CHECK_MESSAGE( edit_history.size() == edit_history_size_after, "Expected   " << edit_history_size_after << " edit history but found " << edit_history.size());
	      }
		}

		BOOST_REQUIRE_MESSAGE( fixtureDef.defsfile_.suiteVec().empty(),"Expected all Suites to be deleted but found " << fixtureDef.defsfile_.suiteVec().size());
	}
}

// Will break for valgrind hence commented out
//BOOST_AUTO_TEST_CASE( test_delete_cmd_for_defs )
//{
//   std::string path_to_very_large_defs_file = "/var/tmp/ma0/BIG_DEFS/3199.def";
//   cout << "Base:: ...test_delete_cmd_for_defs " << path_to_very_large_defs_file;
//
//   // Ok will only run locally, so don't fail, for other platforms
//   if (!fs::exists(path_to_very_large_defs_file))  cout << " ... missing test\n";
//   else {
//      Defs defs;
//      {
//         DurationTimer duration_timer;
//         DefsStructureParser checkPtParser( &defs, path_to_very_large_defs_file);
//         std::string errorMsg,warningMsg;
//         BOOST_REQUIRE_MESSAGE(checkPtParser.doParse(errorMsg,warningMsg),"failed to parse 3199.def");
//         cout << " ...Loading took " << duration_timer.duration();
//         BOOST_CHECK_MESSAGE( duration_timer.duration() < 20,"Loading defs "
//                              << path_to_very_large_defs_file << " took " << duration_timer.duration() << " Expected to take < 20 seconds");
//      }
//
//      DurationTimer duration_timer;
//      MockServer mockServer(&defs);
//      PathsCmd cmd(PathsCmd::DELETE,"",true);
//      cmd.setup_user_authentification(UserCmd::get_user(),string());
//      STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
//      BOOST_CHECK_MESSAGE(returnCmd->ok(),"Failed to delete defs");
//      cout << " ...Deleting took " << duration_timer.duration() << "\n";
//      BOOST_CHECK_MESSAGE( duration_timer.duration() < 2,"Deleting defs "
//                           << path_to_very_large_defs_file << " took " << duration_timer.duration() << " Expected to take < 2 seconds");
//   }
//}

BOOST_AUTO_TEST_SUITE_END()

