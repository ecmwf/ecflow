#ifndef trigger_lister_H
#define trigger_lister_H
//=============================================================================================
// Name        : 
// Author      : 
// Revision    : $Revision: #5 $ 
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


class trigger_lister {
public:

    trigger_lister() {}
 
    enum { normal    = 0,   // Normal trigger_node
           parent    = 1,   // Through parent
           child     = 2,   // Through child
	   hierarchy = 3    // Through child
    }; 

    virtual void next_node(node&, node*,int,node*) = 0;
    virtual Boolean parents()          { return False; }
    virtual Boolean kids()             { return False; }
    virtual Boolean self()             { return True; }

private:
	trigger_lister(const trigger_lister&);
	trigger_lister& operator=(const trigger_lister&);
};

inline void destroy(trigger_lister**) {}
#endif
