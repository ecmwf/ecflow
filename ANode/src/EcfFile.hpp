#ifndef ECF_FILE_HPP_
#define ECF_FILE_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "NodeFwd.hpp"

/// This class is used in the pre-processing of files( .ecf or .usr or .man typically)
/// It is used to to create the job file.
///
/// Please note the %manual is only created on request from the file cmd.
/// Even then it is extracted as a string. i.e. no .man file is
/// created. This is left to the client.
/// When returning the manual we pre-process the files first
///
/// However for testing purpose this capability may be retained.
class EcfFile {
public:
	/// use default copy constructor, assignment, destructor
	/// ECF_FETCH/SMSFETCH is mainly used by research. i.e to obtain scripts and includes
	/// from the version control system. This has not been implemented yet.
	EcfFile(Node*, const std::string& path_to_script_or_fetch_cmd, bool fetchCommand =  false);

	// The path to the ecf file, empty path means that ecf file could not be located
	const std::string& path() const { return script_path_or_cmd_;}

	/// This function will return the contents of %manual -> %end for the input file
	/// It will pre-process the file, then extract the manual form all the pre-processed files
 	/// Will throw std::runtime_error for errors
	void manual(std::string& theManual);

	/// returns the script
	/// Will throw std::runtime_error for errors
	void script(std::string& theScript) const;

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
	void pre_process(std::vector<std::string> & user_edit_file, std::string& pre_processed_file);

	/// Searches for the first %comment, then extracts all variables of the form
	///   <name>  =  <value>
	/// and places into map
	static void extract_used_variables(NameValueMap& used_variables_as_map,const std::vector<std::string> &script_lines);

private:
	enum Type { SCRIPT, INCLUDE, MANUAL, COMMENT };
	static std::string fileType(Type);

	bool open_script_file(const std::string& file, Type, std::vector<std::string>& lines, std::string& errormsg) const;
	bool preProcess(std::vector<std::string>& script_lines, std::string& errormsg);
	bool replaceSmsChildCmdsWithEcf(const std::string& clientPath, std::string& errormsg);
	std::string getIncludedFilePath( const std::string& include, const std::string& line, std::string& errormsg);
 	void variableSubstituition(JobsParam&);
 	const std::string&  doCreateJobFile(JobsParam&) const;
 	bool doCreateManFile(std::string& errormsg);
 	bool extractManual(const std::vector< std::string >& lines, std::vector< std::string >& theManualLines, std::string& errormsg) const;
 	void removeCommentAndManual();
 	void remove_nopp_end_tokens();

 	static int countEcfMicro(const std::string& line, const std::string& ecfMicro);
 	static void dump_expanded_script_file(size_t i, const std::vector<std::string>& lines); // for DEBUG

 	/// returns the extension, i.e for task->.ecf for alias->.usr, will throw if node_ is not task or alias
 	const std::string& get_extn() const;

/// User edit functions:
 	void get_used_variables(std::string& used_variables) const;
 	bool get_used_variables(NameValueMap& used_variables, std::string& errorMsg) const;
 	void doCreateUsrFile() const;

	Node* node_;                         // Task or Alias or Container when pre-processing the man files
	std::string  ecfMicroCache_;         // cache value of ECF_MICRO
	std::string  script_path_or_cmd_;    // path to .ecf, .usr file or command
	mutable std::string  job_size_;     // to be placed in log file during job submission
	//bool         fetchCommand_;        // script is to be extracted form version management repository. Not used !!!
	std::vector<std::string> jobLines_;  // Lines that will form the job file.
};

#endif
