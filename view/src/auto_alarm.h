#ifndef _auto_alarm_h
#define _auto_alarm_h
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2016 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/


#include "setjmp.h"

class auto_alarm {
 private:
  auto_alarm (const auto_alarm&); /* no copy allowed */
  auto_alarm& operator= (const auto_alarm&);

  typedef void (*proc)(int);
  proc (old_);
  int saveSec_;

  static sigjmp_buf env_;
  static bool caught_;
  static int  sec_;

  static void sigAlarm (int);

 public:
  auto_alarm (int);
  
  ~auto_alarm ();

  static bool caught () { return caught_;};
};

#endif
