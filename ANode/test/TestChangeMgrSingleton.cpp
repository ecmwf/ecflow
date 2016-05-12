/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2016 ECMWF.
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
#include "AbstractObserver.hpp"

using namespace std;
using namespace ecf;

class MyObserver : public AbstractObserver {
public:
   MyObserver(Defs* defs) : update_count_(0),defs_(defs),node_(NULL) { defs->attach(this); }
   MyObserver(Node* node) : update_count_(0),defs_(NULL),node_(node) { node->attach(this); }

   virtual ~MyObserver() {
      /* std::cout << "~MyObserver()\n"; */
      if (defs_) defs_->detach(this);
      if (node_) node_->detach(this);
   }

   virtual void update(const Node*, const std::vector<ecf::Aspect::Type>&){update_count_++;}
   virtual void update(const Defs*, const std::vector<ecf::Aspect::Type>&){update_count_++;}

   /// After this call, the node will be deleted, hence observers must *NOT* use the pointers
   virtual void update_delete(const Node* node) {
      //std::cout << "update_delete(const Node* node)\n";
      const_cast<Node*>(node)->detach(this);
   }
   virtual void update_delete(const Defs* defs) {
      //std::cout << "update_delete(const Defs* node)\n";
      const_cast<Defs*>(defs)->detach(this);
   }

   int update_count() const { return update_count_;}
private:
   int update_count_;
   Defs* defs_;
   Node* node_;
};


BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_change_mgr_singleton )
{
   cout << "ANode:: ...test_change_mgr_singleton\n";
   {
      defs_ptr theDefs = Defs::create();

      {
         // ensure defs pointer out-lives the observer
         MyObserver defs_obs(theDefs.get());

         std::vector<ecf::Aspect::Type> aspects;
         theDefs->notify(aspects);
         theDefs->notify(aspects);
         theDefs->notify(aspects);
         theDefs->notify(aspects);
         theDefs->notify(aspects);
         BOOST_CHECK_MESSAGE( defs_obs.update_count() == 5,"Expected 5 update");
      }

      theDefs.reset();
   }

   {
      // **** Note using node_ptr can extend the life of the Node, hence we use scoping ***
      Defs* theDefs = new Defs;
      std::vector<MyObserver*> obs_vec;
      {
         {
            suite_ptr suite = theDefs->add_suite( "suite1" );
            family_ptr fam = suite->add_family( "family" );
            fam->add_task( "t1" );
         }

         // get all nodes and observer them.
         std::vector<node_ptr> node_vec; theDefs->get_all_nodes(node_vec);

         // Need to make sure life time of observer is greater than Node tree
         for(size_t i = 0; i < node_vec.size(); ++i) {
            obs_vec.push_back( new MyObserver( node_vec[i].get() ) );
         }

         // Do some updates
         std::vector<ecf::Aspect::Type> aspects;
         for(size_t i = 0; i < node_vec.size(); ++i) {
            node_vec[i]->notify(aspects);
            node_vec[i]->notify(aspects);
         }
         for(size_t i = 0; i < obs_vec.size(); ++i) {
            BOOST_CHECK_MESSAGE( obs_vec[i]->update_count() == 2,"Expected 2 updates");
         }

         // delete observers
         for(size_t i = 0; i < obs_vec.size(); ++i) { delete  obs_vec[i]; }
      }

      // make sure no node_ptr are in scope as they can *delay*
      // the destructor to the end of the scope, and hence affect this test
      delete theDefs;
   }
}


BOOST_AUTO_TEST_SUITE_END()
