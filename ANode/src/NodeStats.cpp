/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #251 $
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <sstream>
#include "NodeStats.hpp"
using namespace std;

std::string NodeStats::print() const
{
   stringstream ss;
   ss << "Definition statistics\n";
   ss << "Nodes               " << nodes_  << "\n";
   ss << "Suites              " << suites_ << "\n";
   ss << "Family              " << family_ << "\n";
   ss << "Task                " << task_   << "\n";
   ss << "Alias               " << alias_  << "\n\n";
   if (suites_ + family_ +  task_ + alias_ != nodes_) ss << "suites_ + family_ +  task_ + alias_ != nodes_ ?\n";

   ss << "Edit history nodes  " << edit_history_nodes_ << "\n";
   ss << "Edit history paths  " << edit_history_paths_ << "\n\n";

   ss << "vars                " << vars_       << "\n";
   ss << "triggers            " << trigger_    << "\n";
   ss << "complete triggers   " << c_trigger_  << "\n";
   ss << "events              " << events_     << "\n";
   ss << "labels              " << labels_     << "\n";
   ss << "meters              " << meters_     << "\n\n";

   ss << "times               " << times_  << "\n";
   ss << "todays              " << todays_ << "\n";
   ss << "crons               " << crons_  << "\n";
   ss << "dates               " << dates_  << "\n";
   ss << "days                " << days_   << "\n\n";

   ss << "late                " << late_     << "\n";
   ss << "inlimits            " << inlimits_ << "\n";
   ss << "limits              " << limits_   << "\n";
   ss << "repeats             " << repeats_  << "\n";
   ss << "zombies             " << zombies_  << "\n\n";

   ss << "auto_cancel         " << auto_cancel_  << "\n";
   ss << "auto_archive        " << auto_archive_ << "\n";
   ss << "auto_restore        " << auto_restore_ << "\n";
   ss << "verifys             " << verifys_      << "\n";
   ss << "queues              " << queues_       << "\n";
   ss << "generics            " << generics_     << "\n";
   return ss.str();
}
