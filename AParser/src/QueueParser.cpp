//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $
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

#include "QueueParser.hpp"
#include "QueueAttr.hpp"
#include "PrintStyle.hpp"
#include "DefsStructureParser.hpp"
#include "Node.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;

bool QueueParser::doParse(
         const std::string& line,
         std::vector<std::string >& lineTokens )
{
   if ( nodeStack().empty()) {
      throw std::runtime_error("QueueParser::doParse: Could not add queue, as node stack is empty at line: " + line );
   }

   bool parse_state =  false;
   if (rootParser()->get_file_type() != PrintStyle::DEFS) parse_state = true;

   QueueAttr queue_attr;
   QueueAttr::parse(queue_attr,line,lineTokens,parse_state);
   Node* node = nodeStack_top();
   node->add_queue(queue_attr);

   return true;
}
