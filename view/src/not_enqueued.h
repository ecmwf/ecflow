#ifndef not_enqueued_H
#define not_enqueued_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : 
//=============================================================================================


#include "node_alert.h"

class node;

class not_enqueued : public node_alert<not_enqueued> {
public:
	not_enqueued();
	~not_enqueued(); // Change to virtual if base class

protected:

private:

// No copy allowed

	not_enqueued(const not_enqueued&);
	not_enqueued& operator=(const not_enqueued&);

        //

	virtual bool keep(node*);

};

inline void destroy(not_enqueued**) {}

#endif
