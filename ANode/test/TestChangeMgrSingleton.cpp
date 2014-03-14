/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <stdlib.h>
#include <iostream>
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "ChangeMgrSingleton.hpp"
#include "AbstractObserver.hpp"

using namespace std;
using namespace ecf;

class MyObserver : public AbstractObserver {
public:
   MyObserver(Defs* defs) : update_count_(0) { ChangeMgrSingleton::instance()->attach(defs,this); }
   MyObserver(Node* node) : update_count_(0) { ChangeMgrSingleton::instance()->attach(node,this); }

   virtual ~MyObserver() {/* std::cout << "~MyObserver()\n"; */ }

   virtual void update(const Node*, const std::vector<ecf::Aspect::Type>&){update_count_++;}
   virtual void update(const Defs*, const std::vector<ecf::Aspect::Type>&){update_count_++;}

   /// After this call, the node will be deleted, hence observers must *NOT* use the pointers
   virtual void update_delete(const Node* node) {
      //std::cout << "update_delete(const Node* node)\n";
      ChangeMgrSingleton::instance()->detach(const_cast<Node*>(node),this);
   }
   virtual void update_delete(const Defs* defs) {
      //std::cout << "update_delete(const Defs* node)\n";
      ChangeMgrSingleton::instance()->detach(const_cast<Defs*>(defs),this);
   }

   int update_count() const { return update_count_;}
private:
   int update_count_;
};


BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_change_mgr_singleton )
{
   cout << "ANode:: ...test_change_mgr_singleton\n";
   {
      defs_ptr theDefs = Defs::create();

      MyObserver defs_obs(theDefs.get());
      BOOST_CHECK_MESSAGE(ChangeMgrSingleton::instance()->no_of_def_observers() == 1,"Expected one observer");

//      MyObserver defs_obs2(theDefs.get());
//      MyObserver defs_obs3(theDefs.get());
//      BOOST_CHECK_MESSAGE(ChangeMgrSingleton::instance()->no_of_def_observers() == 3,"Expected 3 observer");

      ChangeMgrSingleton::instance()->notify(theDefs);
      ChangeMgrSingleton::instance()->notify(theDefs);
      ChangeMgrSingleton::instance()->notify(theDefs);
      ChangeMgrSingleton::instance()->notify(theDefs);
      ChangeMgrSingleton::instance()->notify(theDefs);
      BOOST_CHECK_MESSAGE( defs_obs.update_count() == 5,"Expected 5 update");
//      BOOST_CHECK_MESSAGE( defs_obs2.update_count() == 5,"Expected 5 update");
//      BOOST_CHECK_MESSAGE( defs_obs3.update_count() == 5,"Expected 5 update");

      theDefs.reset();
      BOOST_CHECK_MESSAGE(ChangeMgrSingleton::instance()->no_of_def_observers() == 0,"Expected no observer");
   }

   {
      // **** Note using node_ptr can extend the life of the Node, hence we use scoping ***
      Defs* theDefs = new Defs;
      std::vector<MyObserver*> obs_vec;
      {
         {
            suite_ptr suite = theDefs->add_suite( "suite1" );
            family_ptr fam = suite->add_family( "family" );
            task_ptr t1 = fam->add_task( "t1" );
         }

         // get all nodes and observer them.
         std::vector<node_ptr> node_vec; theDefs->get_all_nodes(node_vec);

         // Need to make sure life time of observer is greater than Node tree
         for(size_t i = 0; i < node_vec.size(); ++i) {
            obs_vec.push_back( new MyObserver( node_vec[i].get() ) );
         }
         BOOST_CHECK_MESSAGE(ChangeMgrSingleton::instance()->no_of_node_observers() == 3,"Expected 3 observer");

//         // Now add another set of observers
//         for(size_t i = 0; i < node_vec.size(); ++i) {
//            obs_vec.push_back( new MyObserver( node_vec[i].get() ) );
//         }
//         BOOST_CHECK_MESSAGE(ChangeMgrSingleton::instance()->no_of_node_observers() == 6,"Expected 6 observer");

         // Do some updates
         for(size_t i = 0; i < node_vec.size(); ++i) {
            ChangeMgrSingleton::instance()->notify(node_vec[i]);
            ChangeMgrSingleton::instance()->notify(node_vec[i]);
         }
         for(size_t i = 0; i < obs_vec.size(); ++i) {
            BOOST_CHECK_MESSAGE( obs_vec[i]->update_count() == 2,"Expected 2 updates");
         }
      }

      // make sure no node_ptr are in scope as they can *delay*
      // the destructor to the end of the scope, and hence affect this test
      delete theDefs;
      BOOST_CHECK_MESSAGE(ChangeMgrSingleton::instance()->no_of_node_observers() == 0,"Expected no observer but found " << ChangeMgrSingleton::instance()->no_of_node_observers());

      for(size_t i = 0; i < obs_vec.size(); ++i) { delete  obs_vec[i]; }
   }

   // keep valgrind happy
   ChangeMgrSingleton::destroy();
}


BOOST_AUTO_TEST_SUITE_END()
