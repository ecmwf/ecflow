#ifndef INLIMIT_HPP_
#define INLIMIT_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #61 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <ostream>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>         // no need to include <string>
#include <boost/serialization/weak_ptr.hpp>

#include "LimitFwd.hpp"


// Inlimit. Multiple inlimits on same Node are logically ANDED
//    inlimit    limitName     // This will consume one token in the limit <limitName>
//    inlimit    limitName 10  // This will consume 10 tokens in the limit <limitName>
//    inlimit -n limitName     // Only applicable to a family, does not matter how many tasks
//                             // the family has, will only consume one token in the family
//                             // This is like showing that family is active/ has at least
//                             // one task that is submitted <<<<NOT SUPPORTED YET>>>>>>
//
// Inlimit of the same name specified on a task take priority over the family
class InLimit {
public:
   explicit InLimit(const std::string& name,
           const std::string& pathToNode = std::string(),
           int tokens = 1);
   InLimit() : tokens_(1)    {}

   std::ostream& print(std::ostream&) const;
   bool operator==(const InLimit& rhs) const;

   const std::string& name() const { return  name_;}             // must be defined
   const std::string& pathToNode() const { return  pathToNode_;} // can be empty,the node referenced by the In-Limit, this should hold the Limit.
   int tokens() const { return tokens_;}

   std::string toString() const;

private:
   void limit( limit_ptr l) { limit_ = boost::weak_ptr<Limit>(l);}
   Limit* limit() const { return limit_.lock().get();}  // can return NULL
   friend class InLimitMgr;

private:
   std::string          name_;
   std::string          pathToNode_;
   int                  tokens_;
   boost::weak_ptr<Limit>  limit_;     // NOT persisted since computed on the fly

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & name_;
      ar & pathToNode_; // can be empty
      ar & tokens_;
   }
};

#endif
