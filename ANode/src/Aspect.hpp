#ifndef ASPECT_HPP_
#define ASPECT_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
// This class is used to provide observers with more info regarding
// whats changed. used as a part of the observer pattern
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8


#include <boost/noncopyable.hpp>

namespace ecf {

class Aspect : private boost::noncopyable {
public:
   enum Type { NOT_DEFINED, ORDER, ADD_REMOVE_NODE, ADD_REMOVE_ATTR,
               METER_CHANGED, EVENT_CHANGED, LABEL_CHANGED, LIMIT_CHANGED };

private:
   Aspect();
};

}

#endif
