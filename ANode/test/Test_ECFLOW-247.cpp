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
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Jobs.hpp"
#include "JobsParam.hpp"
#include "PrintStyle.hpp"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_ECFLOW_247 )
{
   cout << "ANode:: ...test_ECFLOW_247  \n";

   defs_ptr defs = Defs::create();
   suite_ptr s1 = defs->add_suite("s1");
   family_ptr f1 = s1->add_family("f1");
   f1->add_complete("f1/t1 == complete");
   task_ptr t1 = f1->add_task("t1");
   task_ptr t2 = f1->add_task("t2");
   task_ptr t3 = f1->add_task("t3");
   task_ptr t4 = f1->add_task("t4");

   PrintStyle::setStyle( PrintStyle::MIGRATE);
   {
      defs->beginAll();
      t1->set_state(NState::COMPLETE);
      //cout << defs;

      t4->set_state(NState::ABORTED);
      t4->flag().set(ecf::Flag::FORCE_ABORT); // stopped bu user mimic, otherwise it be be queued

      Jobs jobs(defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);
      //cout << defs;

      BOOST_CHECK_MESSAGE(f1->state() == NState::ABORTED,"The complete for family should not evaluate if child is aborted");
   }
   {
      defs->requeue();
      t1->set_state(NState::COMPLETE);
      //cout << defs;

      t4->set_state(NState::ACTIVE);

      Jobs jobs(defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      //cout << defs;
      BOOST_CHECK_MESSAGE(f1->state() == NState::ACTIVE,"The complete for family should not evaluate if child is active");
   }

   {
      defs->requeue();
      t1->set_state(NState::COMPLETE);

      Jobs jobs(defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      t4->set_state(NState::SUBMITTED);

      BOOST_CHECK_MESSAGE(f1->state() == NState::SUBMITTED,"The complete for family should not evaluate if child is submitted");
      //cout << defs;
   }

   {
      defs->requeue();
      t1->set_state(NState::COMPLETE);
      //cout << defs;

      Jobs jobs(defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      BOOST_CHECK_MESSAGE(f1->state() == NState::COMPLETE,"The complete for family should not evaluate if child is submitted");
      //cout << defs;
   }
}

BOOST_AUTO_TEST_SUITE_END()
