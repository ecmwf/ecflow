//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #67 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <assert.h>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "QueueAttr.hpp"
#include "Ecf.hpp"
#include "Str.hpp"
#include "PrintStyle.hpp"
#include "Indentor.hpp"
#include "Extract.hpp"

using namespace std;
using namespace ecf;

/////////////////////////////////////////////////////////////////////////////////////////////

const QueueAttr& QueueAttr::EMPTY() { static const QueueAttr queueAttr = QueueAttr(); return queueAttr; }
QueueAttr& QueueAttr::EMPTY1() { static QueueAttr queueAttr = QueueAttr(); return queueAttr; }

QueueAttr::QueueAttr(const std::string& name,const std::vector<std::string>& theQueue)
  : used_in_trigger_(false),currentIndex_(0),state_change_no_(0),name_(name),theQueue_(theQueue)
{
   string msg;
   if ( !Str::valid_name( name, msg ) ) {
      throw std::runtime_error( "QueueAttr::QueueAttr: Invalid queue name : " + msg );
   }
   if (theQueue.empty()) {
      throw std::runtime_error( "QueueAttr::QueueAttr: No queue items specified");
   }
   for(size_t i=0; i < theQueue.size(); i++) state_vec_.push_back(NState::QUEUED);
}

QueueAttr::~QueueAttr() {}

std::ostream& QueueAttr::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << toString();
   if ( !PrintStyle::defsStyle() ) {
      if (currentIndex_ != 0)  {
         os << " # " <<  currentIndex_;
         for(size_t i=0; i <  state_vec_.size(); i++) {
            os << " " << state_vec_[i];
         }
      }
   }
   os << "\n";
   return os;
}

bool QueueAttr::operator==(const QueueAttr& rhs) const
{
   if (name_ != rhs.name_) return false;
   if (theQueue_ != rhs.theQueue_) return false;
   if (state_vec_  != rhs.state_vec_) return false;
   if (currentIndex_ != rhs.currentIndex_) return false;
   return true;
}

std::string QueueAttr::value() const
{
   if ( currentIndex_ >=0 && currentIndex_ < static_cast<int>(theQueue_.size())) {
      return theQueue_[currentIndex_];
   }
   return "<NULL>";
}

int QueueAttr::index_or_value() const
{
   if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(theQueue_.size()) ) {
      try {
         return boost::lexical_cast<int>( theQueue_[currentIndex_] );
      }
      catch ( boost::bad_lexical_cast& ) {
         // Ignore and return currentIndex_
      }
   }
   return currentIndex_;
}

NState::State QueueAttr::state(const std::string& step) const
{
   for(size_t i=0; i < theQueue_.size(); i++) {
       if (step == theQueue_[i]) {
          if (i >= state_vec_.size()) throw std::runtime_error("QueueAttr::state: index out of range");
          return state_vec_[i];
       }
    }
    throw std::runtime_error("QueueAttr::state: could not find step " + step);
    return NState::UNKNOWN;
}

void QueueAttr::requeue()
{
   currentIndex_ = 0;
   for(size_t i=0; i < state_vec_.size(); i++) state_vec_[i] = NState::QUEUED;
   incr_state_change_no();
}

std::string QueueAttr::active()
{
   if ( currentIndex_ >=0 && currentIndex_ < static_cast<int>(theQueue_.size())) {
      state_vec_[currentIndex_] = NState::ACTIVE;
      std::string ret = theQueue_[currentIndex_];
      currentIndex_++;
      incr_state_change_no();
      return ret;
   }
   return "<NULL>";
}

void QueueAttr::complete(const std::string& step)
{
   for(size_t i=0; i < theQueue_.size(); i++) {
      if (step == theQueue_[i]) {
         state_vec_[i] = NState::COMPLETE;
         incr_state_change_no();
         return;
      }
   }
   std::stringstream ss; ss << "QueueAttr::complete: Could not find " << step << " in queue " << name_;
   throw std::runtime_error(ss.str());
}

void QueueAttr::aborted(const std::string& step)
{
   for(size_t i=0; i < theQueue_.size(); i++) {
      if (step == theQueue_[i]) {
         state_vec_[i] = NState::ABORTED;
         incr_state_change_no();
         return;
      }
   }
   std::stringstream ss; ss << "QueueAttr::aborted: Could not find " << step << " in queue " << name_;
   throw std::runtime_error(ss.str());
}

std::string QueueAttr::no_of_aborted() const
{
   int count = 0;
   for(size_t i=0; i < state_vec_.size(); i++) {
      if (state_vec_[i] == NState::ABORTED) count++;
   }
   if (count !=0) return boost::lexical_cast<std::string>(count);
   return std::string();
}

void  QueueAttr::reset_index_to_first_queued_or_aborted()
{
   for(size_t i=0; i < state_vec_.size(); i++) {
      if (  state_vec_[i] == NState::QUEUED || state_vec_[i] == NState::ABORTED) {
         currentIndex_ = i;
         incr_state_change_no();
         break;
      }
   }
}

std::string QueueAttr::toString() const
{
   std::string ret;
   ret = "queue ";
   ret += name_;
   for(size_t i = 0; i < theQueue_.size(); i++) {
      ret += " ";
      ret += theQueue_[i];
   }
   return ret;
}

std::string QueueAttr::dump() const
{
   std::stringstream ss;
   ss << toString() << " # " << currentIndex_;
   for(size_t i=0; i < state_vec_.size(); i++) ss << " " << state_vec_[i];
   return ss.str();
}

void QueueAttr::incr_state_change_no()
{
   state_change_no_ = Ecf::incr_state_change_no();
}

void QueueAttr::parse(QueueAttr& queAttr, const std::string& line, std::vector<std::string >& lineTokens, bool parse_state )
{
   size_t line_tokens_size = lineTokens.size();
   if ( line_tokens_size < 3 ) {
      std::stringstream ss;
      ss << "QueueAttr::parse: expected at least 3 tokens, found " << line_tokens_size << " on line:" << line << "\n";
      throw std::runtime_error(ss.str());
   }

   // queue name "first" "second" "last" #   current_index state state state
   //   0     1        2       3        4    5   6
   queAttr.set_name(lineTokens[1]);

   std::vector<std::string> theEnums; theEnums.reserve(line_tokens_size);
   for(size_t i = 2; i < line_tokens_size; i++) {
      std::string theEnum = lineTokens[i];
      if (theEnum[0] == '#') break;
      Str::removeSingleQuotes(theEnum);// remove quotes, they get added back when we persist
      Str::removeQuotes(theEnum);      // remove quotes, they get added back when we persist
      theEnums.push_back(theEnum);
   }
   if ( theEnums.empty() ) throw std::runtime_error( "queue: has no values " + line );


   int index = 0;
   std::vector<NState::State> state_vec;

   if (parse_state) {
      // queue VARIABLE a b c # index state state state
      for(size_t i = 0; i < lineTokens.size(); i++) {
         if (lineTokens[i] == "#" && i+1 < lineTokens.size()) {
            i++;
            index = Extract::theInt(lineTokens[i] ,"QueueAttr::parse, could not extract index");
            i++;
            for(;i < lineTokens.size(); i++) {
               int state = Extract::theInt(lineTokens[i] ,"QueueAttr::parse, could not extract state");
               state_vec.push_back(static_cast<NState::State>(state));
            }
            break;
         }
      }
   }

   queAttr.set_queue(theEnums,index,state_vec);
}

void QueueAttr::set_queue( const std::vector<std::string>& theQueue,int index,const std::vector<NState::State>& state_vec)
{
   if (theQueue.empty()) throw std::runtime_error( "QueueAttr::set_queue: No queue items specified");

   if (!state_vec.empty()) {
      if (state_vec.size() != theQueue.size()) {
         std::stringstream ss;
         ss << "QueueAttr::set_state: for queue " << name_ << " size " << theQueue.size() << " does not match state size " << state_vec.size();
         throw std::runtime_error(ss.str());
      }
      state_vec_ = state_vec;
   }
   else {
      for(size_t i=0; i < theQueue.size(); i++) state_vec_.push_back( NState::QUEUED );
   }

   currentIndex_ = index;
   theQueue_ = theQueue;
}

void QueueAttr::set_state_vec(const std::vector<NState::State>& state_vec)
{
   state_vec_ = state_vec;
   if (theQueue_.size() != state_vec_.size()) {
      std::cout << "QueueAttr::set_state_vec: for queue " << name_ << " queue size " << theQueue_.size() << " not equal to state_vec size " << state_vec_.size() << "\n";
   }
}

void QueueAttr::set_name( const std::string& name)
{
   string msg;
   if ( !Str::valid_name( name, msg ) ) {
      throw std::runtime_error( "QueueAttr::set_name: Invalid queue name : " + msg );
   }
   name_ =  name;
}
