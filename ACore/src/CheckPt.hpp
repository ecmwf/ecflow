#ifndef CHECKPT_HPP_
#define CHECKPT_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
// This class is used to ONLY serialise the edit history when check pointing
// Also Provides enums for check pointing
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

namespace ecf {

class CheckPt  {
public:
   /// NEVER   - the check pt file is never saved
   /// ON_TIME - the check pt file is saved periodically. specified by checkPtInterval.
   /// ALWAYS  - the check pt file is saved after any state change
   /// UNDEFINED   - Internal use only
   enum Mode { NEVER, ON_TIME, ALWAYS, UNDEFINED};

   /// The interval between automatic saves of check point by server
   static int default_interval() { return 120;}

   /// If saving check point takes longer than the alarm time, raise late flag on the server
   static int default_save_time_alarm() { return 20;}

private:
   CheckPt() = delete;
   CheckPt(const CheckPt&) = delete;
   const CheckPt& operator=(const CheckPt&) = delete;
};

}
#endif
