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
  : currentIndex_(0),state_change_no_(0),name_(name),theQueue_(theQueue)
{
   string msg;
   if ( !Str::valid_name( name, msg ) ) {
      throw std::runtime_error( "QueueAttr::QueueAttr: Invalid queue name : " + msg );
   }
   if (theQueue.empty()) {
      throw std::runtime_error( "QueueAttr::QueueAttr: No queue items specified");
   }
}

QueueAttr::~QueueAttr() {}

std::ostream& QueueAttr::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << toString();
   if ( !PrintStyle::defsStyle() ) {
      if (currentIndex_ != 0)  os << " # " <<  currentIndex_;
   }
   os << "\n";
   return os;
}

bool QueueAttr::operator==(const QueueAttr& rhs) const
{
   if (name_ != rhs.name_) return false;
   if (theQueue_ != rhs.theQueue_) return false;
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

void QueueAttr::reset()
{
   currentIndex_ = 0;
   incr_state_change_no();
}

void QueueAttr::increment()
{
   currentIndex_++;
   incr_state_change_no();
}

std::string QueueAttr::toString() const
{
   std::string ret;
   ret = "queue ";
   ret += name_;
   for(size_t i = 0; i < theQueue_.size(); i++) {
      ret += " ";
      ret += theQueue_[i] ;
   }
   return ret;
}

std::string QueueAttr::dump() const
{
   std::stringstream ss;
   ss << toString() << " # " << currentIndex_;
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

   // queue name "first" "second" "last" #   3
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
   queAttr.set_queue(theEnums);

   int index = 0; // This is *assumed to be the index* and not the value

   if (parse_state) {
      // search back for comment
      // queue VARIABLE a b c # value
      std::string token_after_comment;
      for(size_t i = line_tokens_size-1; i > 3; i--) {
         if (lineTokens[i] == "#") {
            // token after comment is the index
            index = Extract::theInt(token_after_comment,"QueueAttr::parse, could not extract index");
            break;
         }
         else token_after_comment = lineTokens[i];
      }
   }
   queAttr.set_index(index);
}

void QueueAttr::set_name( const std::string& name)
{
   string msg;
   if ( !Str::valid_name( name, msg ) ) {
      throw std::runtime_error( "QueueAttr::set_name: Invalid queue name : " + msg );
   }
   name_ =  name;
}

void QueueAttr::set_queue( const std::vector<std::string>& theQueue)
{
   if (theQueue.empty()) {
      throw std::runtime_error( "QueueAttr::set_queue: No queue items specified");
   }
   theQueue_ = theQueue;
}

