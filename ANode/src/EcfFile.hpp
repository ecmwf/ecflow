#ifndef ECF_FILE_HPP_
#define ECF_FILE_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/noncopyable.hpp>
#include "boost/filesystem/path.hpp"
#include <boost/shared_ptr.hpp>
#include <fstream>
#include "NodeFwd.hpp"

// This class is used to minimise file I/0.
// When job processing the same include file can be opened and closed many time
// This cache serves to open the include file only once. It halves the job processing time.
class IncludeFileCache : private boost::noncopyable {
public:
   explicit IncludeFileCache(const std::string& path);
   ~IncludeFileCache();
   const std::string& path() const { return path_;}
   bool lines(std::vector<std::string>&);
private:
   std::string path_;
   std::ifstream fp_;   // is not copy constructable
   size_t no_of_lines_;
};

/// This class is used in the pre-processing of files( .ecf or .usr or .man typically)
/// It is used to to create the job file.
///
///
/// Please note the %manual is only created on request from the file cmd.
/// Even then it is extracted as a string. i.e. no .man file is
/// created. This is left to the client.
/// When returning the manual we pre-process the files first
///
/// However for testing purpose this capability may be retained.

class EcfFile {
public:
   enum Origin { ECF_SCRIPT,     // Look for .ecf file, uses default algorithm to find %includes
                 ECF_HOME,       // default directory for scripts
                 ECF_FILES,      // User specified directory
                 ECF_FETCH_CMD,  // pre-process output of ECF_FETCH,     all %includes use same command
                 ECF_SCRIPT_CMD  // pre-process output of ECF_SCRIPT_CMD, uses default algorithm to find %includes
             };

   // Use to record how the ecf_file was located from ECF_FILES and ECF_HOME directories. Default is PRUNE_ROOT
   enum EcfFileSearchAlgorithm {
      PRUNE_ROOT,
      PRUNE_LEAF
   };

   EcfFile();

   /// This preserves the caches, used to avoid opening/stat of include file more than once.
	EcfFile& operator=(const EcfFile& rhs);

	/// use default copy constructor, assignment, destructor
	/// ECF_FETCH  is used obtain the script from running a command  i.e.
	/// from the version control system.
	EcfFile(Node*, const std::string& path_to_script_or_fetch_cmd, EcfFile::Origin = ECF_SCRIPT, EcfFile::EcfFileSearchAlgorithm = PRUNE_ROOT);

	// The path to the ecf file, empty path means that ecf file could not be located
	bool valid() const { return !script_path_or_cmd_.empty();}

	/// This function will return the contents of %manual -> %end for the input file
	/// It will pre-process the file, then extract the manual form all the pre-processed files
 	/// Will throw std::runtime_error for errors
	void manual(std::string& theManual);

	/// returns the script, The first line contains the origin of the file/cmd.
	/// Will throw std::runtime_error for errors
	void script(std::string& theScript) const;

    const std::string&  script_path_or_cmd() const {return script_path_or_cmd_;}

	/// What is the origin of this ecf_file
	std::string ecf_file_origin_dump() const;

	/// simple accessors
	Origin ecf_file_origin() const { return script_origin_;}
	EcfFileSearchAlgorithm ecf_file_search_algorithm() const { return ecf_file_search_algorithm_;}
   static std::string origin_str(Origin);
   static std::string search_algorithm_str(EcfFileSearchAlgorithm);

	/// Create the job file from with script of a task or alias.
	/// Note: the location of the ecf file may not be the same as the job file
	///       For creating the job we must use ECF_JOB
	///
	/// _IF_ ECF_CLIENT is specified provide Special support for migration. TEST ONLY
 	/// The variable ECF_CLIENT is used to specify the path to client exe.
	/// This is then used to replace smsinit,smscomplete, smsevent,smsmeter.smslabel,smsabort
	/// This function will start the pre processing
	/// Will throw std::runtime_error for errors
	const std::string& create_job( JobsParam&);

	/// Process the script file, to add all the used variables, add the start of the file
	/// between %comment %end, The augmented script is returned in file_with_used_variables
	/// Will throw std::runtime_error for errors
	void edit_used_variables(std::string& file_with_used_variables);

	/// Pre-processing involves include expansion and variable substitution
	/// Will throw std::runtime_error if pre processing fails
	void pre_process(std::string& pre_processed_file);

   /// Does full pre-processing:
   ///   - include pre-processing
   ///   - variable substitution
   ///   - remove comment and manual sections
   ///   - ignore %noop
	void pre_process_user_file(std::vector<std::string> & user_edit_file, std::string& pre_processed_file);

	/// Searches for the first %comment, then extracts all variables of the form
	///   <name>  =  <value>
	/// and places into map
	static void extract_used_variables(NameValueMap& used_variables_as_map,const std::vector<std::string> &script_lines);

private:
	friend class PreProcessor;
	enum Type { SCRIPT, INCLUDE, MANUAL, COMMENT };
	static std::string fileType(EcfFile::Type);

	bool open_script_file(const std::string& file, EcfFile::Type, std::vector<std::string>& lines, std::string& errormsg) const;
	bool open_include_file(const std::string& file,std::vector<std::string>& lines,std::string& errormsg) const;

	bool replaceSmsChildCmdsWithEcf(const std::string& clientPath, std::string& errormsg);
 	void variableSubstitution(JobsParam&);
 	const std::string&  doCreateJobFile(JobsParam&) const;
 	bool doCreateManFile(std::string& errormsg);
 	bool extractManual(const std::vector< std::string >& lines, std::vector< std::string >& theManualLines, std::string& errormsg) const;
 	void removeCommentAndManual();
 	void remove_nopp_end_tokens();

 	static int countEcfMicro(const std::string& line, const std::string& ecfMicro);
 	static void dump_expanded_script_file(const std::vector<std::string>& lines); // for DEBUG

 	/// returns the extension, i.e for task->.ecf for alias->.usr, will throw if node_ is not task or alias
 	const std::string& get_extn() const;

 	bool do_popen(const std::string& the_cmd, EcfFile::Type, std::vector<std::string>& lines, std::string& errormsg) const;

   boost::filesystem::path file_creation_path() const;
   std::string script_or_job_path() const;

/// User edit functions:
 	void get_used_variables(std::string& used_variables) const;
 	bool get_used_variables(NameValueMap& used_variables, std::string& errorMsg) const;
 	void doCreateUsrFile() const;
 	bool file_exists(const std::string& file_path) const;

 	Node* node_;                         // Task or Alias or Container when pre-processing the man files
 	std::string  ecfMicroCache_;         // cache value of ECF_MICRO
 	std::string  script_path_or_cmd_;    // path to .ecf, .usr file or command
 	std::vector<std::string> jobLines_;  // Lines that will form the job file.
 	mutable std::vector<boost::shared_ptr<IncludeFileCache> > include_file_cache_; // only open include file once
 	mutable std::vector<std::pair<std::string,bool> > file_stat_cache_; // Minimise calls to stat/kernel calls
 	mutable std::string  job_size_;      // to be placed in log file during job submission
 	EcfFile::Origin      script_origin_; // get script from a file, or from running a command
 	EcfFile::EcfFileSearchAlgorithm ecf_file_search_algorithm_; // only used for ECF_FILES and ECF_HOME
};


// This class is used in expanding(pre-processing) the includes.
// The pre-processing is done in a depth first fashion (ECFLOW-673)
class PreProcessor : private boost::noncopyable {
public:
   explicit PreProcessor(EcfFile*);
   ~PreProcessor();

   bool preProcess(std::vector<std::string>& script_lines );
   const std::string& error_msg() const { return error_msg_;}

private:
   // include pre-processing on the included file.
   // Note: include directives _in_ manual/comment should he handled.
   //       only include directives in %nopp/%end are ignored
   void preProcess_line(const std::string& script_line );
   void preProcess_includes(const std::string& script_line);
   std::string getIncludedFilePath( const std::string& include, const std::string& line);

private:
   EcfFile* ecfile_;

   bool nopp_;
   bool comment_;
   bool manual_;

   std::string pp_nopp_;
   std::string pp_comment_;
   std::string pp_manual_;
   std::string pp_end_;
   std::string ecf_micro_;                 // constant until ecfmicro changes, then reset

   std::vector<std::string>& jobLines_;
   std::vector<std::string> tokens_;       // re-use to save memory

   std::vector<std::pair<std::string,int> > globalIncludedFileSet_;// test for recursive includes, <no _of times it was included>
   std::vector<std::string> include_once_set_;
   std::string error_msg_;
};

#endif
