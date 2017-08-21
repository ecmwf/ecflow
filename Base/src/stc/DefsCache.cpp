/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #55 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include "DefsCache.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "Family.hpp"
#include "Suite.hpp"
#include "Alias.hpp"
#include "Ecf.hpp"
#include "Log.hpp"
#include "boost_archive.hpp" // collates boost archive includes
#include "Serialization.hpp"

using namespace std;
using namespace boost;

// =====================================================================================================
//#define DEBUG_SERVER_SYNC 1
//#define DEBUG_CLIENT_SYNC 1

// ===========================================================================================
// CACHE: the deserialization costs, so that if multiple clients request the full defs
//        we can improve the performance, by only performing that once for each state change.
std::string DefsCache::full_server_defs_as_string_ = "";
unsigned int DefsCache::state_change_no_ = 0;
unsigned int DefsCache::modify_change_no_= 0;

void DefsCache::update_cache_if_state_changed(defs_ptr defs)
{
   // See if there was a state change *OR* if cache is empty
   if (state_change_no_ != Ecf::state_change_no() ||
       modify_change_no_ != Ecf::modify_change_no() ||
       full_server_defs_as_string_.empty()
      )
   {
      try {
#ifdef DEBUG_SERVER_SYNC
         cout << ": *updating* cache";
#endif
         defs->save_as_string(full_server_defs_as_string_,PrintStyle::MIGRATE); // update cache
      }
      catch ( std::exception& ae ) {
         // Unable to decode data. Something went wrong, inform the caller.
         ecf::LogToCout logToCout;
         LOG(ecf::Log::ERR,"DefsCache::update_cache_if_state_changed exception " << ae.what());
         throw;
      }

      state_change_no_ = Ecf::state_change_no();
      modify_change_no_ =  Ecf::modify_change_no();
   }
#ifdef DEBUG_SERVER_SYNC
   else {
      cout << ": *cache* up to date";
   }
#endif
}

defs_ptr DefsCache::restore_defs_from_string(const std::string& archive_data)
{
#ifdef DEBUG_CLIENT_SYNC
   cout << ": DefsCache::restore_defs_from_string: archive_data.size(" << archive_data.size() << ")";
#endif

   defs_ptr defs = Defs::create();
   try {
      // cout << "archive_data:\n" << archive_data << "\n";
      defs->restore_from_string(archive_data);
   }
   catch (std::exception& e) {
      ecf::LogToCout logToCout;
      LOG(ecf::Log::ERR,"DefsCache::restore_defs_from_string " << e.what());
      throw;
   }

#ifdef DEBUG_CLIENT_SYNC
   if (defs.get()) cout << ": valid defs";
   else            cout << ": *empty* defs?";
#endif
   return defs;
}

defs_ptr DefsCache::restore_defs_from_string()
{
   // Used in Test when no client/server
   return restore_defs_from_string(full_server_defs_as_string_);
}

