//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #7 $ 
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

#include "show.h"
#include "globals.h"
option<int> show::status32_ (globals::instance(), "show_mask32", 0);

option<int> show::status_ (globals::instance(), "show_mask", 
			     (1<<show::unknown)    
			     |(1<<show::suspended) 
			     |(1<<show::complete) 
			     |(1<<show::queued) 
			     |(1<<show::submitted) 
			     |(1<<show::active)   
			     |(1<<show::aborted)  
			     |(1<<show::time_dependant) 
			     |(1<<show::late_nodes)    
  			     |(1<<show::migrated_nodes)
			     |(1<<show::rerun_tasks)   
			     |(1<<show::nodes_with_messages) 
			     |(1<<show::label)     
			     |(1<<show::meter)    
			     |(1<<show::event)   
			     |(1<<show::repeat)   
			     |(1<<show::time)   
			     |(1<<show::date)   
			     |(1<<show::late)   
			     |(1<<show::inlimit)   
			     |(1<<show::limit)   
			     |(1<<show::trigger)
			     //	& (~(1<<show::variable))
			     //	& (~(1<<show::genvar))
			     |(1<<show::time_icon) 
			     |(1<<show::date_icon) 
			     |(1<<show::late_icon) 
			     |(1<<show::waiting_icon) 
			     |(1<<show::rerun_icon) 
			     |(1<<show::migrated_icon) 
			     |(1<<show::message_icon)
 // & (~(1<<show::defstatus_icon)) & (~(1<<show::zombie_icon))
			     ); 

show::show(int f) : flag_(f) {
  status_ = status_ & (~(1<<show::variable));
  status_ = status_ & (~(1<<show::genvar));
}

show::~show() {}

void show::on()
{
  if (flag_ > 31) {
    status32_ = int(status32_) | (1<<(flag_-32));
  } else {
    status_   = int(status_  ) | (1<<(flag_));
  }
}

void show::off()
{
  if (flag_ == show::all) { 
    status_ = 0xFFFF ; status32_ = 0xFFFF;
    status32_ = (int) (status32_) & (~(1<<(show::none-32)));
    status32_ = (int) (status32_) & (~(1<<(show::all -32))); 
  } else if (flag_ == show::none) {
    status_ = 0; status32_ = 0; 
  } else if (flag_ > 31) {
    status32_ = int(status32_) & (~(1<<(flag_-32))); 
  } else { 
    status_   = int(status_)   & (~(1<<flag_)); 
  }
}

bool show::wanted()
{
  if (flag_ > 31) {
    return (int(status32_) & (1<<(flag_-32))) != 0;
  } else {
    return (int(status_  ) & (1<<flag_))    != 0;
  }
}
