//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #3 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================

#include <signal.h>

#include "ecflowview.h"
#include "auto_alarm.h"

int auto_alarm::sec_    = 0;

auto_alarm::auto_alarm (int sec)
{
  printf ("set alarm %d\n",sec);  
  old_ = ::signal (SIGALRM, sigAlarm);
  sec_ = sec;

  ::alarm (sec);
};


auto_alarm::~auto_alarm ()
{
  sec_ = saveSec_;
  ::signal (SIGALRM, old_);
  ::alarm (0);
  printf ("reset alarm\n");  
};


void auto_alarm::sigAlarm (int)
{
  printf ("catching\n");  
  /* if (recover_jump) */
  // FILL longjmp(ecf_jump_station, 1);
};
