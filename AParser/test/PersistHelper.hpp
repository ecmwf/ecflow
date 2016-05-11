#ifndef PERSISTHELPER_HPP_
#define PERSISTHELPER_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================


#include <boost/noncopyable.hpp>
#include <string>
#include "Archive.hpp"
#include "PrintStyle.hpp"
class Defs;

/// Given a in memory defs file, this class will write it to disk
/// and reload the definition file structure and will then make a comparison
/// to ensure they are the same
class PersistHelper : private boost::noncopyable {
public:
	PersistHelper(bool compare_edit_history = false) : file_size_(0),compare_edit_history_(compare_edit_history) {}

	bool test_persist_and_reload( const Defs& theInMemoryDefs, PrintStyle::Type_t file_type_on_disk);
	bool test_checkpt_and_reload( const Defs& theInMemoryDefs, bool do_compare = true,ecf::Archive::Type at = ecf::Archive::default_archive());
	bool test_state_persist_and_reload_with_checkpt( const Defs& theInMemoryDefs );
	const std::string& errorMsg() const { return errorMsg_;}

	/// returns the file size of the temporary file created by:
	///   test_persist_and_reload(..) or test_checkpt_and_reload(..)
	size_t file_size() const { return file_size_;}

private:

   bool reload_from_defs_file( const Defs& theInMemoryDefs, Defs& reloaded_defs, const std::string& filename );
   bool reload_from_checkpt_file(const Defs& theInMemoryDefs,
                                 Defs& reloaded_defs,
                                 bool do_compare = true,
                                 ecf::Archive::Type at = ecf::Archive::default_archive() );

private:

	std::string errorMsg_;
	size_t file_size_;
	bool compare_edit_history_;
};
#endif



