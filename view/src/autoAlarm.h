#ifndef _AutoAlarm_h
#define _AutoAlarm_h
/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #4 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2019 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/


class AutoAlarm {
 private:
  AutoAlarm (const AutoAlarm&); /* no copy allowed */
  AutoAlarm& operator= (const AutoAlarm&);

  typedef void (*proc)(int);
  proc (old_);
  int saveSec_;

  static sigjmp_buf env_;
  static bool caught_;
  static int  sec_;

  static void sigAlarm (int);

 public:
  AutoAlarm (int);
  
  ~AutoAlarm ();

  static bool caught () { return caught_;};
};

#endif
