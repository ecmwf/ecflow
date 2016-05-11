#ifndef show_H
#define show_H
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

// #include <inttypes.h>

#ifndef option_H
#include "option.h"
#endif

#ifdef WITH_INT128
#define VLONG uint128
#else
// #define VLONG uint64_t
// #define VLONG unsigned long long 
// #define VLONG __int64 
#define VLONG int 
#endif


class show {
 	int flag_;
        static option< VLONG > status_;
        static option< VLONG > status32_;

public:

	enum {
		unknown ,
		suspended ,
		complete ,
		queued ,
		submitted ,
		active ,
		aborted ,
		time_dependant ,
		late_nodes ,
		waiting_nodes ,
		migrated_nodes ,
		rerun_tasks ,
		nodes_with_messages ,
		label ,
		meter,
		event ,
		repeat ,
		time ,
		date ,
		late ,
		trigger,
		variable,
		genvar,
		limit,
		inlimit,
		time_icon,
		date_icon,
		late_icon,
		waiting_icon,
		rerun_icon,
		migrated_icon,
		message_icon,

                defstatus_icon, zombie_icon, // time_holding_icon, date_holding_icon
                all, none
	};

	// show(int f) : flag_(f) {};
 show(int f);
	~show();

	void on();
	void off();
	bool wanted();

	static inline bool want(int flag) { 
	  return (flag > 31) ? 
	    (int(status32_) & (1<<(flag-32))) != 0 : 
	    (int(status_) & (1<<flag))!=0; }

	 static inline bool want32(int flag)  
	{ return (flag > 31) ? 
	    (int(status32_) & (1<<(flag-32))) != 0 : 
	    false; }

	static inline VLONG status() { return int(status_); }

	VLONG flag() const { return flag_; }
private:
	show(const show&);
	show& operator=(const show&);
};
#endif
