#ifndef FILE_HPP_
#define FILE_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #41 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class for file utilities
//============================================================================

#include <boost/noncopyable.hpp>
#include <boost/filesystem/path.hpp>
#include <string>
#include <vector>


namespace ecf {

class File : private boost::noncopyable {
public:

	static size_t MAX_LINES();  //  max number of lines, default to 10000
	static const std::string& JOB_EXTN();  // ".job"
	static const std::string& MAN_EXTN();  // ".man"
	static const std::string& USR_EXTN();  // ".usr"
	static const std::string& ECF_EXTN();  // ".ecf"

	/// Search for the file, in $PATH return the first path that matches or an empty file, if not
	static std::string which(const std::string& file);

	/// expect string of the form ".ecf" or ".sms"
	static void set_ecf_extn(const std::string&);
	static const std::string& ecf_extn(); // return ".ecf" | ".sms" | ".py"

	/// return the file extension
	static std::string getExt(const std::string& file);

	/// replace file extension with a new one
	static void replaceExt(std::string& file, const std::string& newExt);

	/// returns the input files, split into a vector of string, where each string
	/// represent a line in the file. returns true if file open ok , false otherwise
	/// The additional parameter ignoreEmptyLine  can be used to ignore empty lines
	/// **Always reads _WHOLE_ file. Not suitable for very large files **
	static bool splitFileIntoLines(const std::string& filename, std::vector<std::string>& lines, bool ignoreEmptyLine = false);

	/// This is suitable for large files. > several gigabytes, since it does load the entire file
   static std::string get_last_n_lines(const std::string& filename,int last_n_lines, std::string& error_msg);
   static std::string get_last_n_lines(const std::string& filename,int last_n_lines, size_t& file_size,std::string& error_msg);

	/// returns the first n line of a file, does not read all the file, hence suitable for very large files
	static std::string get_first_n_lines(const std::string& filename,int n_lines, std::string& error_msg);

	/// Opens the file and returns the contents
	static bool open(const std::string& filePath, std::string& contents);

	/// Given a file spath, and a vector of lines, creates a file. returns true if success
	/// else returns false and an error message
	static bool create(const std::string& filename, const std::vector<std::string>& lines, std::string& errorMsg);
	static bool create(const std::string& filename, const std::string& contents, std::string& errorMsg);

	/// recursively look for a file, given a starting directory
	/// Return the first file that matches
	/// return true if file found false otherwise
	static bool find(
			const boost::filesystem::path& dir_path,    // from this directory downwards
			const std::string&             file_name,   // search for this name,
			boost::filesystem::path&       path_found   // placing path here if found
	);

	/// recursively look for a file, given a starting directory
	/// Returns _ALL_ files that match
	static void findAll(
			const boost::filesystem::path& dir_path,            // from this directory downwards
			const std::string&             file_name,           // search for this name,
			std::vector<boost::filesystem::path>& paths_found   // placing path here if found
	);


	/// recursively look for a file, given a starting directory and path token
	/// Returns the first match found
	static std::string findPath(
			const boost::filesystem::path& dir_path,    // from this directory downwards
			const std::string&             file_name,   // search for this name,
			const std::string&             leafDir      // path must contain this string
	);

	static std::string findPath(
			const boost::filesystem::path&  dir_path,    // from this directory downwards
			const std::string&              file_name,   // search for this name,
			const std::vector<std::string>& tokens       // path must contain all these tokens
	);

	/// Create missing directories. This is *NOT* the same as boost::create_directories
	/// as that only works with directories. This function assumes that if a "." exist
	/// in the string it represents a file.
	/// Hence this function handles:
	///   /tmp/some/dir/fred.job       // i.e the directories  /tmp/some/dir/ will be created
	///   /tmp/some/dir
	///   fred                         // will create the directory fred
	///   fred.job                     // just return true
	static bool createMissingDirectories(const std::string& pathToFileOrDir);

	/// Create directories the boost way, with additional check to see if directories exist first
	static bool createDirectories(const std::string& pathToDir);

	/// Returns the difference between 2 files.
	/// Ignore lines that contain strings in the ignoreVec
	static  std::string diff(const std::string& file,
			const std::string& file2,
			const std::vector<std::string>& ignoreVec,
			std::string& errorMsg,
			bool ignoreBlanksLine  = true);

	/// Do a backward search of rootPath + nodePath + fileExtn
	/// If task path if of the form /suite/family/family2/task, then we keep
	/// on consuming the first path token this should leave:
	///   	<root-path>/suite/family/family2/task.ecf
	///   	<root-path>/family/family2/task.ecf
	///  	<root-path>/family2/task.ecf
	///   	<root-path>/task.ecf
	/// See page 21 of SMS user guide
	//
	/// Returns an empty string if file not found
	static std::string backwardSearch( const std::string& rootPath, const std::string& nodePath, const std::string& fileExtn );

	// Remove a directory recursively ****
	static bool removeDir( const boost::filesystem::path& p);

	// Locate the path to the server exe
	static std::string find_ecf_server_path();

	// Locate the path to the client exe
	static std::string find_ecf_client_path();

	// Locate test data
	static std::string test_data(const std::string& rel_path, const std::string& dir);

	// return root source
	static std::string root_source_dir();
	static std::string root_build_dir();


   static int max_open_file_allowed();

};

}

#endif /* FILE_HPP_ */
