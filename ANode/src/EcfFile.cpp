//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #66 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <set>
#include <sstream>
#include <sys/stat.h>

#include "boost/foreach.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/algorithm/string/trim.hpp>

#include "EcfFile.hpp"
#include "Log.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "File.hpp"
#include "Task.hpp"
#include "JobsParam.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;
using namespace boost;

//#define DEBUG_ECF_ 1
//#define DEBUG_PRE_PROCESS 1
//#define DEBUG_PRE_PROCESS_INCLUDES 1    // be careful with this as it will skew  the diffs in test code
//#define DEBUG_MIGRATE 1                 // for TEST ONLY

//#define DEBUG_PRE_PROCESS_OUTPUT 1
//#define DEBUG_VAR_SUB_OUTPUT 1
//#define DEBUG_MAN_FILE 1

static const char* T_NOOP        = "nopp";
static const char* T_COMMENT     = "comment";
static const char* T_MANUAL      = "manual";
static const char* T_END         = "end";
static const char* T_ECFMICRO    = "ecfmicro";
static const char* T_INCLUDE     = "include";
static const char* T_INCLUDENOPP = "includenopp";

static void vector_to_string(const std::vector<std::string>& vec, std::string& str)
{
   // Determine size of string, to avoid reallocation
   size_t the_string_size = 0;
   size_t theSize = vec.size();
   for(size_t i = 0; i < theSize; i++) { the_string_size += vec[i].size() + 1; } // +1 is for "\n";
   str.reserve( str.size() + the_string_size);

   // populate string using the vector
   for(size_t i = 0; i < theSize; i++) {
      str += vec[i];
      str += "\n";
   }
}

// ==================================================================================
// Avoid making any data model/defs state changes. Changes like:
//       node_->flag().set(ecf::Flag::NO_SCRIPT);
// Which will cause a state change.
// If the script fails then it is up to the calling Task to set this flag
// This class is also used to extract the scripts/manual, etc form
// a read only command. hence this class can not make state changes
// ===================================================================================
EcfFile::EcfFile( Node* t,
                  const std::string& pathToEcfFileOrCommand,
                  bool  fetchCommand
)
: node_( t ),
  script_path_or_cmd_( pathToEcfFileOrCommand )
  /*,fetchCommand_( fetchCommand ) */
{
   node_->findParentUserVariableValue(Str::ECF_MICRO(),ecfMicroCache_);
   if ( ecfMicroCache_.empty() || ecfMicroCache_.size() != 1) {
      std::stringstream ss;
      ss << "EcfFile::EcfFile: Node " << t->absNodePath() << " is referencing a invalid ECF_MICRO variable(' " << ecfMicroCache_ << "). ECF_MICRO when overridden, must be a single character.";
      throw std::runtime_error(ss.str());
   }

#ifdef DEBUG_ECF_
   cout << "   EcfFile::EcfFile pathToEcfFileOrCommand = " << script_path_or_cmd_ << " fetchCommand = " << fetchCommand << "\n";
#endif
}

void EcfFile::manual(std::string& theManual)
{
   /// Pre-process the file accessible from the server
   std::vector<std::string> lines;
   std::string error_msg;
   EcfFile::Type file_type = (node_->isSubmittable()) ? EcfFile::SCRIPT : EcfFile::MANUAL ;
   if (!open_script_file(script_path_or_cmd_, file_type, lines,  error_msg)) {
      std::stringstream ss; ss << "EcfFile::manual: For node " << node_->debugNodePath() << ", failed to open file " << script_path_or_cmd_ << " : " << error_msg;
      throw std::runtime_error(ss.str());
   }

   // expand all %includes this will expand %includenopp by enclosing in %nopp %end, will populate jobLines_
   if (!preProcess(lines,error_msg)) {
      std::stringstream ss; ss << "EcfFile::manual: For node " << node_->debugNodePath() << ", failed to pre-process file " << script_path_or_cmd_ << " : " << error_msg;
      throw std::runtime_error(ss.str());
   }

   // perform variable sub's but don't error if failure
   try {
      JobsParam dummy; // create jobs = false, spawn jobs =  false
      variableSubstituition(dummy);
   }
   catch (...) {}

   vector<string> theManualLines;
   if (!extractManual(jobLines_,theManualLines,error_msg)) {
      std::stringstream ss;
      ss << "EcfFile::manual: extraction failed for task " << node_->absNodePath() << " " << error_msg;
      throw std::runtime_error(ss.str());
   }

   if (theManualLines.empty()) {
      // There is no %manual -> %end in the file. However this may be .man file for Suites/Family
      // For this case just include the pre-processed contents as is, ie since the whole file
      // is the manual
      if (node_->isNodeContainer()) {
         vector_to_string(jobLines_,theManual);
         return;
      }
   }

   vector_to_string(theManualLines,theManual);
}

void EcfFile::script(std::string& theScript) const
{
   if (!File::open(script_path_or_cmd_,theScript)) {
      std::stringstream ss;
      ss << "EcfFile::script: Could not open script for task/alias " << node_->absNodePath() << " at path " << script_path_or_cmd_;
      throw std::runtime_error(ss.str());
   }
}

void EcfFile::pre_process(std::vector<std::string>& user_edit_file, std::string& pre_processed_file)
{
   // expand all %includes this will expand %includenopp by enclosing in %nopp %end, will populate jobLines_
   std::string errormsg;
   if (!preProcess(user_edit_file,errormsg)) {
      throw std::runtime_error("EcfFile::pre_process: Failed to pre_process user edit file " + errormsg);
   }

   vector_to_string(jobLines_,pre_processed_file);
}

void EcfFile::pre_process(std::string& pre_processed_file)
{
   /// Pre-process the ECF file accessible from the server
   std::vector<std::string> lines;
   std::string error_msg;
   if (!open_script_file(script_path_or_cmd_, EcfFile::SCRIPT, lines,  error_msg)) {
      std::stringstream ss;
      ss << "EcfFile::pre_process: Failed to open file " << script_path_or_cmd_ << " : " << error_msg;
      throw std::runtime_error(ss.str());
   }

   // expand all %includes this will expand %includenopp by enclosing in %nopp %end, will populate jobLines_
   if (!preProcess(lines,error_msg)) {
      throw std::runtime_error("EcfFile::pre_process: Failed to pre_process: " + error_msg);
   }

   /// Find Used variables, *after* all %includes expanded, can throw std::runtime_error
   get_used_variables(pre_processed_file);

   /// Add pre-processed content
   vector_to_string(jobLines_,pre_processed_file);
}

void EcfFile::edit_used_variables(std::string& return_script_with_used_variables)
{
   std::string errorMsg;
   std::vector<std::string> lines;
   if (!open_script_file(script_path_or_cmd_, EcfFile::SCRIPT, lines,  errorMsg)) {
      throw std::runtime_error( "EcfFile::edit_used_variables: Open script failed : " + errorMsg ) ;
   }

   // Copy the script file, *BEFORE* expanding the includes
   std::string script;
   vector_to_string(lines,script);

   // expand all %includes
   if (!preProcess(lines, errorMsg)) {
      throw std::runtime_error( "EcfFile::edit_used_variables: PreProcess script failed : " + errorMsg ) ;
   }

   /// Find Used variables, *after* all %includes expanded, Can throw std::runtime_error
   get_used_variables(return_script_with_used_variables);

   /// Return Used variables and SCRIPT before pre-processing
   return_script_with_used_variables += script;
}


const std::string& EcfFile::create_job( JobsParam& jobsParam)
{
#ifdef DEBUG_ECF_
   cout << "EcfFile::createJob task " << node_->absNodePath() << " script_path_or_cmd_ = " << script_path_or_cmd_ << "\n";
#endif

   // NOTE: When editing pure python jobs, we may have *NO* variable specified, but only user_edit_file
   //       hence whenever we have user_edit_file, we should follow the else part below
   std::string error_msg;
   std::vector<std::string> lines;
   if (jobsParam.user_edit_variables().empty() && jobsParam.user_edit_file().empty()) {
      /// The typical *NORMAL* path
      if (!open_script_file(script_path_or_cmd_, EcfFile::SCRIPT, lines,  error_msg)) {
         throw std::runtime_error("EcfFile::create_job: failed " + error_msg );
      }
   }
   else {
      // *USER* edit, two kinds
      if (jobsParam.user_edit_file().empty()) {
         // *USE* user variables, but ECF file accessible from the server
         if (!open_script_file(script_path_or_cmd_, EcfFile::SCRIPT, lines,  jobsParam.errorMsg())) {
            throw std::runtime_error("EcfFile::create_job: User variables, Could not open script: " + error_msg );
         }
      }
      else {
         // *USE* the user supplied ECF file *AND* user variables
         lines = jobsParam.user_edit_file();
      }
   }

   // expand all %includes this will expand %includenopp by enclosing in %nopp %end
   if (!preProcess(lines,error_msg)) {
      throw std::runtime_error("EcfFile::create_job: pre process failed " + error_msg );
   }

#ifdef DEBUG_PRE_PROCESS_OUTPUT
   std::string err;
   File::create("preProcess" + get_extn(),jobLines_,err);
#endif

   // _IF_ ECF_CLIENT is specified provide Special support for migration.
   // The variable ECF_CLIENT is used to specify the path to client exe.
   // This is then used to replace smsinit,smscomplete, smsevent,smsmeter.smslabel,smsabort
   std::string clientPath;
   if (node_->findParentUserVariableValue("ECF_CLIENT", clientPath)) {
      if (!replaceSmsChildCmdsWithEcf(clientPath,error_msg) ) {
         throw std::runtime_error("EcfFile::create_job: ECF_CLIENT replacement failed " + error_msg );
      }
#ifdef DEBUG_MIGRATE
      std::string err;
      File::create("migrate" + get_extn(),jobLines_,err);
#endif
   }

   /// Will use *USER* supplied edit variables in preference to node tree variable *IF* supplied
   /// expand %VAR% or %VAR:sub% & replace %% with %
   // Allow variable substitution in comment and manual blocks. But if it fails, don't report as an error
   variableSubstituition(jobsParam);

#ifdef DEBUG_VAR_SUB_OUTPUT
   std::string err1;
   File::create("variableSub" + get_extn(),jobLines_,err1);
#endif

#ifdef DEBUG_MAN_FILE
   if (!doCreateManFile(error_msg))  {
      throw std::runtime_error("EcfFile::create_job: manual file creation failed " + error_msg );
   }
#endif

   /// Create the user file for tasks, when submitting without aliases,  before removing comments
   if (node_->isTask() && !jobsParam.user_edit_variables().empty()) {
      doCreateUsrFile();
   }

   removeCommentAndManual();
   remove_nopp_end_tokens();

   return doCreateJobFile(jobsParam/* this is only past in for profiling */); // create job on disk
}

void EcfFile::extract_used_variables(NameValueMap& used_variables_as_map,const std::vector<std::string> &script_lines)
{
   // we only process the contents of the FIRST %comment  %end
   bool comment = false;
   size_t theSize = script_lines.size();
   for(size_t i=0; i < theSize; ++i) {

      // std::cout << "EcfFile::extract_used_variables found:'" << script_lines[i] << "\n";
      if (script_lines[i].empty()) continue;

      // take into account micro char during variable substitution
      string::size_type ecfmicro_pos = script_lines[i].find(Ecf::MICRO());
      if ( ecfmicro_pos == 0) {

         // We can not do variable substitution between %nopp/%end
         if (script_lines[i].find(T_COMMENT) == 1)  { comment = true; continue;}
         if (script_lines[i].find(T_NOOP)    == 1)  { return;}
         if (script_lines[i].find(T_MANUAL)  == 1)  { return;}
         if (script_lines[i].find(T_END)     == 1)  { return; }
      }

      if (comment) {

         // expect  name =  value
         string::size_type equal_pos = script_lines[i].find("=");
         if ( equal_pos == string::npos) continue;
         string name = script_lines[i].substr(0,equal_pos);
         string value = script_lines[i].substr(equal_pos+1);
         boost::algorithm::trim(name);
         boost::algorithm::trim(value);

         //std::cout << "   extracted as '" << name << "' = '" << value << "'\n";
         used_variables_as_map.insert( std::make_pair(name, value) );
      }
   }
}

bool EcfFile::preProcess(std::vector<std::string>& script_lines, std::string& errormsg)
{
   /// Clear existing jobLines
   jobLines_.clear();
   jobLines_.reserve(512); // estimate for includes

   // get the cached ECF_MICRO variable, typically its one char.
   string ecfMicro = ecfMicroCache_;

#ifdef DEBUG_PRE_PROCESS
   cout << "   EcfFile::preProcess task:" << node_->absNodePath() << "   script_path_or_cmd_=" << script_path_or_cmd_ << " ecfMicro = " << ecfMicro << "\n";
   for(size_t i=0; i < script_lines.size(); ++i) { cerr << "   script_lines[i] = " << i << "  " << script_lines[i] << "\n"; }
#endif

   // include pre-processing on the included file.
   // Note: include directives _in_ manual/comment should he handled.
   //       only include directives in %nopp/%end are ignored
   std::set<std::string> globalIncludedFileSet; // test for recursive includes
   int recursive_count = 0;
   std::vector<std::string> tokens;       // re-use to save memory
   std::vector<std::string> includeLines; // re-use to save memory

   // constant until ecfmicro changes, then reset
   string pp_nopp = ecfMicro;    pp_nopp    += T_NOOP;
   string pp_comment = ecfMicro; pp_comment += T_COMMENT;
   string pp_manual = ecfMicro;  pp_manual  += T_MANUAL;
   string pp_end = ecfMicro;     pp_end     += T_END;

   while (1) {

      std::set<std::string> localIncludedFileSet;
      bool filesToInclude = false;
      bool nopp =  false; bool comment = false; bool manual = false;
      for(size_t i=0; i < script_lines.size(); ++i) {
         jobLines_.push_back(script_lines[i]);    // copy line

         // For variable substitution % can occur anywhere on the line, for pre -processing of
         // %ecfmicro,%manual,%comment,%end,%include,%includenopp it must be the very *first* character
         string::size_type ecfmicro_pos = script_lines[i].find(ecfMicro);
         if (ecfmicro_pos == string::npos) continue;

         if (!nopp && !comment && !manual) {
            // For variable substitution '%' can occur anywhere on the line.
            // Check for Mismatched micro i.e %FRED or %FRED%%
            if (ecfmicro_pos != 0) {
               int ecfMicroCount = countEcfMicro( script_lines[i], ecfMicro );
               if (ecfMicroCount % 2 != 0 ) {
                  std::stringstream ss;
                  ss << "Mismatched ecfmicro(" << ecfMicro << ") count(" << ecfMicroCount << ")  '" << script_lines[i] << "' in " << path();
                  errormsg += ss.str();
                  dump_expanded_script_file(i,script_lines);
                  return false;
               }
            }
         }

         // %ecfmicro,%manual,%comment,%end,%include,%includenopp it must be the very *first* character
         if (ecfmicro_pos != 0) continue; //handle 'garbage%include'

#ifdef DEBUG_PRE_PROCESS
         std::cout << i << ": " << script_lines[i] << "\n";
#endif
         if (script_lines[i].find(pp_manual) == 0) {
            if (comment || manual) {
               std::stringstream ss; ss << "Embedded comments/manuals not supported '" << script_lines[i] << "' at " << path();
               errormsg += ss.str();
               dump_expanded_script_file(i,script_lines);
               return false;
            }
            manual = true ; continue;
         }
         if (script_lines[i].find(pp_comment) == 0) {
            if (comment || manual) {
               std::stringstream ss; ss << "Embedded comments/manuals not supported '" << script_lines[i] << "' at " << path();
               errormsg += ss.str();
               dump_expanded_script_file(i,script_lines);
               return false;
            }
            comment = true ; continue;
         }
         if (script_lines[i].find(pp_nopp) == 0) {
            if (nopp) {
               std::stringstream ss; ss << "Embedded nopp not supported '" << script_lines[i] << "' in " << path();
               errormsg += ss.str();
               dump_expanded_script_file(i,script_lines);
               return false;
            }
            nopp = true ; continue;
         }
         if (script_lines[i].find(pp_end) == 0) {
            if (comment) { comment = false; continue;}
            if (manual)  { manual = false; continue;}
            if (nopp)    { nopp = false; continue;}
            std::stringstream ss;
            ss << pp_end << " found with no matching %comment | %manual | %nopp at '" << script_lines[i]<< "' at path " << path();
            errormsg += ss.str();
            dump_expanded_script_file(i,script_lines);
            return false;
         }
         if (nopp) continue;

         tokens.clear();
         Str::split( script_lines[i], tokens );

         // Handle ecfmicro replacement ================================================================================
         if (script_lines[i].find(T_ECFMICRO) == 1) {    // %ecfmicro #
            // keep %ecfmicro in jobs file later processing, i.e for comments/manuals

            if (tokens.size() < 2) {
               std::stringstream ss;
               ss << "ecfmicro does not have a replacement character, in " << script_path_or_cmd_;
               errormsg += ss.str();
               return false;
            }

            // This is typically a single character, however $/£ will be multi-character i.e size 2
            ecfMicro = tokens[1];
            if (ecfMicro.size() > 2) {
               std::stringstream ss;
               ss << "Expected ecfmicro replacement to be a single character, but found '" << ecfMicro << "' " <<  ecfMicro.size() << " in file : " << script_path_or_cmd_;
               errormsg += ss.str();
               return false;
            }

            pp_nopp = ecfMicro;    pp_nopp    += T_NOOP;
            pp_comment = ecfMicro; pp_comment += T_COMMENT;
            pp_manual = ecfMicro;  pp_manual  += T_MANUAL;
            pp_end = ecfMicro;     pp_end     += T_END;

            continue;
         }

         // Handle the includes ===================================================================================
         if (tokens.size() < 2) continue;

         bool includenopp = (script_lines[i].find(T_INCLUDENOPP) == 1);
         if (!includenopp) {
            // Notice we only do recursive includes for %include
            filesToInclude = (script_lines[i].find(T_INCLUDE) != string::npos);
         }
         if (!filesToInclude && !includenopp) continue;

         // remove %include since were going to expand it.
         jobLines_.pop_back();

#ifdef DEBUG_PRE_PROCESS_INCLUDES
         // Output the includes for debug purposes. Will appear in preProcess.ecf
         // Note: Will interfere with diff
         jobLines_.push_back("========== include of " + tokens[1] + " ===========================");
#endif

         std::string includedFile = getIncludedFilePath(tokens[1], script_lines[i], errormsg);
         if (!errormsg.empty())  return false;
         localIncludedFileSet.insert(includedFile);
#ifdef DEBUG_PRE_PROCESS
         cout << "EcfFile::preProcess processing " << includedFile  << "\n";
#endif

         includeLines.clear();
         if (!open_script_file(includedFile, EcfFile::INCLUDE, includeLines,  errormsg)) {
            return false;
         }

         // append included script_lines to jobsLines
         if (includenopp) jobLines_.push_back(ecfMicro + T_NOOP);
         std::copy( includeLines.begin(), includeLines.end(), std::back_inserter( jobLines_ ) );
         if (includenopp) jobLines_.push_back(ecfMicro + T_END);
      }


      if (nopp) {
         std::stringstream ss;
         ss << "Unterminated nopp, matching 'end' is missing for " << path();
         errormsg +=  ss.str();
         dump_expanded_script_file(1,script_lines);
         return false;
      }

      // Check for recursive includes. some includes like %include <endt.h>
      // are included many times, but the include is not recursive.
      // To get round this will use a simple count.
      BOOST_FOREACH(const string& theInclude, localIncludedFileSet) {

         if (globalIncludedFileSet.find(theInclude) != globalIncludedFileSet.end()) {

            if ( recursive_count > 10) {
               std::stringstream ss;
               ss << "Recursive include of file " << theInclude << " for " << path();
               errormsg += ss.str();
               return false;
            }
            recursive_count++;
         }
         else globalIncludedFileSet.insert(theInclude);
      }

      if (!filesToInclude)  break;
      else {
         // repeat until no %include left
         script_lines = jobLines_;
         jobLines_.clear();
      }
   }
   return true;
}

bool EcfFile::open_script_file(
         const std::string& file,
         EcfFile::Type type,
         std::vector<std::string>& lines,
         std::string& errormsg) const
{
#ifdef DEBUG_ECF_
   std::cout << "EcfFile::open_script_file file(" << file << ") type(" << fileType(type) << ")\n";
#endif
   if (file.empty()) {
      std::stringstream ss;
      ss << "EcfFile::open_script_file: Could not open ecf " << fileType(type) << " file. Input File/cmd string is empty.";
      errormsg += ss.str();
      return false;
   }

   //	if (fetchCommand_) {
   //		// SMSFETCH is not used in operation or research. I think this is how it should work
   //		// But not tested. Commented out until demand for it.
   //		string theFile = file;
   //		string theCommand = script_path_or_cmd_; // variables have already been substituted
   //		switch (type) {
   //			case EcfFile::SCRIPT:  { theCommand += " -s "; theFile = node_->name() + get_extn(); break;}
   //			case EcfFile::INCLUDE: theCommand += " -i "; break;
   //			case EcfFile::MANUAL:  { theCommand += " -m "; theFile = node_->name() + get_extn(); break;}
   //			case EcfFile::COMMENT: { theCommand += " -c "; theFile = node_->name() + get_extn(); break;}
   //		}
   //		theCommand += theFile;
   //		FILE *fp = popen(theCommand.c_str(),"r");
   //		//cout << " " << theCommand << "\n";
   //		if (!fp) {
   // 			std::stringstream ss;
   //			ss  << "Could not open " <<  fileType(type) << " via cmd " << theCommand << " for task " << node_->absNodePath() << " ";
   //			errormsg += ss.str();
   //			return false;
   //		}
   // 		char  line[LINE_MAX];
   //		while( fgets(line,LINE_MAX,fp) ) {
   // 			lines.push_back(line);
   // 		}
   //		pclose(fp);
   // 	}
   //	else {
   if ( ! File::splitFileIntoLines(file, lines) ) {
      std::stringstream ss;
      ss  << "Could not open " <<  fileType(type) << " file:" << file;
      errormsg += ss.str();
      return false;
   }
   //	}

   return true;
}

std::string EcfFile::fileType(EcfFile::Type t)
{
   switch (t) {
      case EcfFile::SCRIPT:  return "script";  break;
      case EcfFile::INCLUDE: return "include"; break;
      case EcfFile::MANUAL:  return "manual";  break;
      case EcfFile::COMMENT: return "comment"; break;
   }
   assert(false);
   return string();
}


static void replace(  string::size_type commentPos,
         std::string& jobLine,
         const std::string& smsChildCmd,
         const std::string& ecfEquiv,
         const std::string& clientPath)
{
   string::size_type childPos = jobLine.find(smsChildCmd);
   if ( childPos != std::string::npos ) {
      if ( commentPos == std::string::npos) {
         std::string replace = clientPath;
         replace +=  ecfEquiv;
         Str::replace(jobLine,smsChildCmd,replace);
      }
      else if ( childPos < commentPos) {
         std::string replace = clientPath;
         replace +=  ecfEquiv;
         Str::replace(jobLine,smsChildCmd,replace);
      }
   }
}

bool EcfFile::replaceSmsChildCmdsWithEcf(const std::string& clientPath, std::string& errormsg)
{
   //   smsinit $$          	   ---> ECF_CLIENT(value) --init $$
   //   smscomplete         	   ---> ECF_CLIENT(value)
   //   smsevent eventname       ---> ECF_CLIENT(value) --event eventname
   //   smsmeter metername value ---> ECF_CLIENT(value) --meter metername value
   //   smslabel value           ---> ECF_CLIENT(value) --label value
   //   smswait expr             ---> ECF_CLIENT(value) --wait expr
   //   smsabort                 ---> ECF_CLIENT(value) --abort
   size_t jobLines_size = jobLines_.size();
   for(size_t i=0; i < jobLines_size; ++i) {

      // ONLY do the replacement if there is no leading comment
      string::size_type commentPos = jobLines_[i].find("#");
      replace(commentPos, jobLines_[i], "smsinit",    " --init ",     clientPath);
      replace(commentPos, jobLines_[i], "smscomplete"," --complete ", clientPath);
      replace(commentPos, jobLines_[i], "smsabort",   " --abort ",    clientPath);
      replace(commentPos, jobLines_[i], "smsevent",   " --event ",    clientPath);
      replace(commentPos, jobLines_[i], "smsmeter",   " --meter ",    clientPath);
      replace(commentPos, jobLines_[i], "smslabel",   " --label ",    clientPath);
      replace(commentPos, jobLines_[i], "smswait",    " --wait ",     clientPath);
   }
   return true;
}

void EcfFile::variableSubstituition(JobsParam& jobsParam)
{
   // Allow variable substitution in comment and manual blocks.
   // But if it fails, don't report as an error

   // get the cached ECF_MICRO variable, typically its one char.
   string ecfMicro = ecfMicroCache_;
   char microChar = ecfMicro[0];

   // We need a stack to properly implement nopp. This is required since we need to pair
   // the %end, with nopp. i.e need to handle
   // %nopp
   // %comment
   // %end      // this is paired with comment
   // %end      // This is paired with nopp
   const int NOPP = 0;
   const int COMMENT = 1;
   const int MANUAL = 2;
   std::vector<int> pp_stack;
   std::vector<std::string> tokens;

   bool nopp =  false;
   size_t jobLines_size = jobLines_.size();
   for(size_t i=0; i < jobLines_size; ++i) {

      if (jobLines_[i].empty()) continue;

      // take into account micro char during variable substitution
      string::size_type ecfmicro_pos = jobLines_[i].find(ecfMicro);
      if (ecfmicro_pos == 0) {

         // We can not do variable substitution between %nopp/%end
         if (jobLines_[i].find(T_MANUAL)  == 1) { pp_stack.push_back(MANUAL); continue;  }
         if (jobLines_[i].find(T_COMMENT) == 1) { pp_stack.push_back(COMMENT); continue; }
         if (jobLines_[i].find(T_NOOP)    == 1) { pp_stack.push_back(NOPP); nopp = true; continue; }
         if (jobLines_[i].find(T_END) == 1) {
            if (pp_stack.empty()) throw std::runtime_error("EcfFile::variableSubstituition: failed unpaired %end");
            int last_directive = pp_stack.back(); pp_stack.pop_back();
            if (last_directive == NOPP) nopp = false;
            continue;
         }

         if (jobLines_[i].find(T_ECFMICRO) == 1) {   // %ecfmicro #

            tokens.clear();
            Str::split( jobLines_[i], tokens );
            if (tokens.size() < 2) {
               std::stringstream ss; ss << "ecfmicro does not have a replacement character, in " << script_path_or_cmd_;
               throw std::runtime_error("EcfFile::variableSubstituition: failed : " + ss.str());
            }
            ecfMicro = tokens[1];
            microChar = ecfMicro[0];
            continue; // no point in doing variable subs on %ecfmicro ^
         }
      }
      if ( nopp ) continue;


      /// For variable substitution % can occur anywhere on the line
      if (ecfmicro_pos != string::npos) {

         /// In the *NORMAL* flow jobsParam.user_edit_variables() will be EMPTY
         if ( !node_->variable_substitution( jobLines_[i], jobsParam.user_edit_variables(), microChar ) ) {

            // Allow variable substitution in comment and manual blocks.
            // But if it fails, don't report as an error
            int last_directive = -1;
            if (!pp_stack.empty()) last_directive = pp_stack.back();
            if ( last_directive == COMMENT || last_directive == MANUAL) continue;

            std::stringstream ss;  ss << "EcfFile::variableSubstituition: failed : '" << jobLines_[i] << "'";
            dump_expanded_script_file( i, jobLines_ );
            throw std::runtime_error(ss.str());
         }
      }
   }
}


void EcfFile::get_used_variables(std::string& used_variables) const
{
   /// Find Used variables, *after* all %includes expanded
   NameValueMap used_variables_map;
   std::string errorMsg;
   if (!get_used_variables(used_variables_map, errorMsg) ) {
      throw std::runtime_error( "EcfFile::get_used_variables: Extract used variables failed : " + errorMsg ) ;
   }

   if (!used_variables_map.empty()) {

      // add %comment - edit user variable, %end - ecf user variable
      used_variables = ecfMicroCache_;
      used_variables += "comment - ecf user variables\n";

      // ***************************************************************************************
      // Custom handling of dynamic variables, i.e ECF_TRYNO, ECF_PASS and
      // any variable that embeds a try number, i.e. ECF_JOB, ECF_JOBOUT
      // This is required since the try number is *always* incremented *before* job submission,
      // hence the value extracted from the job file will *not* be accurate, hence we exclude it.
      // This way at job submission we use the latest/correct value, which is in-sync with JOB OUTPUT
      // Note: Otherwise the job output will not be in sync
      //
      // Custom handling of ECF_PORT,ECF_NODE,ECF_NAME do not show these variables, these variables
      // including ECF_PASS appear in the script. If the user accidentally edits them,
      // Child communication with the server will be broken. Hence not shown
      //
      // All the above are examples of generated variables, which should not really be edited
      // The used variables are typically *user* variable *in* the scripts, that user may need
      // to modify. Hence we have also excluded generated variables SUITE, FAMILY, TASK
      // ****************************************************************************************
      std::pair<std::string, std::string> item;
      BOOST_FOREACH(item, used_variables_map) {
         if ( item.first.find(Str::ECF_TRYNO())   != std::string::npos) continue;
         if ( item.first.find(Str::ECF_JOB())     != std::string::npos) continue;
         if ( item.first.find(Str::ECF_JOBOUT())  != std::string::npos) continue;
         if ( item.first.find(Str::ECF_PASS())    != std::string::npos) continue;
         if ( item.first.find(Str::ECF_PORT())    != std::string::npos) continue;
         if ( item.first.find(Str::ECF_NODE())    != std::string::npos) continue;
         if ( item.first.find(Str::ECF_NAME())    != std::string::npos) continue;

         // We must use exact match, to avoid user variables like ESUITE,EFAMILY,ETASK
         if ( item.first == Str::TASK())  continue;
         if ( item.first == Str::FAMILY()) continue;
         if ( item.first == "FAMILY1")     continue;
         if ( item.first == Str::SUITE())  continue;
         used_variables += item.first;
         used_variables += " = ";
         used_variables += item.second;
         used_variables += "\n";
      }

      used_variables += ecfMicroCache_;
      used_variables += "end - ecf user variables\n";
   }
}

bool EcfFile::get_used_variables(NameValueMap& used_variables, std::string& errormsg) const
{
   // get the cached ECF_MICRO variable, typically its one char.
   string ecfMicro = ecfMicroCache_;

   char microChar = ecfMicro[0];

   // We need a stack to properly implement nopp. This is required since we need to pair
   // the %end, with nopp. i.e need to handle
   // %nopp
   // %comment
   // %end      // this is paired with comment
   // %end      // This is paired with nopp
   const int NOPP = 0;
   const int COMMENT = 1;
   const int MANUAL = 2;
   std::vector<int> pp_stack;

   bool nopp =  false;
   std::stringstream ss;
   std::vector<std::string> tokens;

   size_t job_lines_size = jobLines_.size();
   for(size_t i=0; i < job_lines_size; ++i) {

      if (jobLines_[i].empty()) continue;

      // take into account micro char during variable substitution
      string::size_type ecfmicro_pos = jobLines_[i].find(ecfMicro);
      if (ecfmicro_pos == 0) {

         // We can not do variable substitution between %nopp/%end
         if (jobLines_[i].find(T_MANUAL)  == 1) { pp_stack.push_back(MANUAL); continue;  }
         if (jobLines_[i].find(T_COMMENT) == 1) { pp_stack.push_back(COMMENT); continue; }
         if (jobLines_[i].find(T_NOOP)    == 1) { pp_stack.push_back(NOPP); nopp = true; continue; }
         if (jobLines_[i].find(T_END) == 1) {
            if (pp_stack.empty()) throw std::runtime_error("EcfFile::get_used_variables: failed  unpaired %end");
            int last_directive = pp_stack.back(); pp_stack.pop_back();
            if (last_directive == NOPP) nopp = false;
            continue;
         }

         if (!nopp && jobLines_[i].find(T_ECFMICRO) == 1) {  // %ecfmicro #

            tokens.clear();
            Str::split( jobLines_[i], tokens );
            if (tokens.size() < 2) {
               std::stringstream ss; ss << "ecfmicro does not have a replacement character, in " << script_path_or_cmd_;
               throw std::runtime_error("EcfFile::get_used_variables: failed : " + ss.str());
            }
            ecfMicro = tokens[1];
            microChar = ecfMicro[0];
            continue;
         }
      }
      if ( nopp ) continue;


      if (ecfmicro_pos != string::npos) {

         /// *Note:* currently this modifies jobLines_[i]
         std::string line_copy = jobLines_[i];  // avoid modifying the jobs Lines, end up doing  variable substitution
         if ( !node_->find_all_used_variables( line_copy, used_variables, microChar ) ) {

            // Allow variable substitution in comment and manual blocks.
            // But if it fails, dont report as an error
            int last_directive = -1;
            if (!pp_stack.empty()) last_directive = pp_stack.back();
            if ( last_directive == COMMENT || last_directive == MANUAL) continue;

            ss << "Variable find failed for '" << jobLines_[i] << "'  microChar='" << microChar << "' ";
            dump_expanded_script_file( i, jobLines_ );
         }
      }
   }

   // Append to error message if any
   errormsg += ss.str();

   return errormsg.empty();
}


const std::string& EcfFile::doCreateJobFile(JobsParam& jobsParam) const
{
   if ( jobLines_.size() > 1 ) {
      // Guard against ecf file that exist's but is empty,
      // no point in creating empty job files for them

      // ECF_JOB is used with ECF_JOB_CMD when submitting a job.
      // First look for a user variable of name ECF_JOB, otherwise look for
      // the generated variable, hence this should never fail.
      // *This* assumes that:
      //   a/ if the user has overiden ECF_JOB then it has been specified at the task level
      //      Otherwise findParentVariableValue will find the generated ECF_JOB on the task
      //   b/ The value of the user variable has a valid directory paths and job file name
      //   c/ The user will lose the try number.
      std::string ecf_job;
      if (!node_->findParentVariableValue(Str::ECF_JOB(), ecf_job)) {
         LOG_ASSERT( !ecf_job.empty() ,"ECF_JOB should have been generated, program error");
      }

      // *** The location of the ECF_ file may not always be the same as the location
      // *** of the job file. Job file location is specified by ECF_JOB
      // Locate the directory where we found the ecf file.
      fs::path script_file_path( script_path_or_cmd_ );
      fs::path parent_path = script_file_path.parent_path();
      if ( fs::is_directory( parent_path ) ) {

         // cout << "EcfFile::createJob ecf " << path() << " ECF_JOB(" << ecf_job << ")\n";
         if (!File::createMissingDirectories(ecf_job)) {
            std::stringstream ss;
            ss << "EcfFile::doCreateJobFile: Could not create missing directories for ECF_JOB " << ecf_job << " File system full?";
            throw std::runtime_error(ss.str());
         }

         // Create the jobs file.
         std::string error_msg;
         if (!File::create(ecf_job, jobLines_,error_msg)) {
            std::stringstream ss;
            ss << "EcfFile::doCreateJobFile: Could not create job file " << ecf_job << " " << error_msg << " File system full?";
            throw std::runtime_error(ss.str());
         }

         // make the job file executable
         if ( chmod( ecf_job.c_str(), 0755 ) != 0 ) {
            std::stringstream ss;
            ss << "EcfFile::doCreateJobFile: Could not make job file " << ecf_job << "  executable by using chmod";
            throw std::runtime_error(ss.str());
         }

         // record job size, for placement into log files
         size_t job_output_size = 0;
         size_t jobLines_size = jobLines_.size();
         for(size_t i = 0; i < jobLines_size; ++i)  job_output_size += jobLines_[i].size();
         job_output_size += jobLines_size; // take into account new lines for each line of output
         job_size_ = "job_size:";
         job_size_ += boost::lexical_cast<std::string>(job_output_size);
         return job_size_;
      }
      else {
         std::stringstream ss;
         ss << "EcfFile::doCreateJobFile: The path '" << script_file_path.parent_path() << "' is not a directory";
         throw std::runtime_error(ss.str());
      }
   }

   std::stringstream ss;
   ss << "EcfFile::doCreateJobFile: The ecf file '" << path() << "' that is associated with task '" << node_->absNodePath() << "' is empty";
   throw std::runtime_error(ss.str());
}

bool EcfFile::doCreateManFile( std::string& errormsg)
{
   vector<string> manFile;
   if (!extractManual(jobLines_,manFile,errormsg)) {
      return false;
   }
   if ( !manFile.empty() ) {

      // find the directory associated with ecf file and place Man file there.
      fs::path script_file_path( script_path_or_cmd_ );
      fs::path parent_path = script_file_path.parent_path();
      if ( fs::is_directory( parent_path ) ) {

         fs::path theManFilePath( parent_path.string() + '/' + node_->name() + File::MAN_EXTN() );

         // cout << "EcfFile::doCreateManFile job " << manFile.string() << "\n";
         if (!File::create(theManFilePath.string(),manFile,errormsg)) return false;
      }
      else {
         std::stringstream ss;
         ss << "man file creation failed. The path '" << script_file_path.parent_path() << "' is not a directory";
         errormsg += ss.str();
         return false;
      }
   }
   return true;
}


void EcfFile::doCreateUsrFile() const
{
   // find the directory associated with ecf file and place .usr file there.
   fs::path script_file_path( script_path_or_cmd_ );
   fs::path parent_path = script_file_path.parent_path();
   if ( fs::is_directory( parent_path ) ) {

      fs::path theUsrFilePath( parent_path.string() + '/' + node_->name() + File::USR_EXTN() );

      // cout << "EcfFile::doCreateUsrFile job " << theUsrFilePath.string() << "\n";
      std::string error_msg;
      if (!File::create(theUsrFilePath.string(),jobLines_,error_msg)) {
         throw std::runtime_error("EcfFile::doCreateUsrFile: file creation failed : " + error_msg);
      }
   }
   else {
      std::stringstream ss;
      ss << "EcfFile::doCreateUsrFile: file creation failed. The path '" << script_file_path.parent_path() << "' is not a directory";
      throw std::runtime_error(ss.str());
   }
}


bool EcfFile::extractManual(const std::vector< std::string >& lines,
         std::vector< std::string >& theManualLines,
         std::string& errormsg) const
{
   // Note: we have already done pre-processing, ie since the manual is obtained after
   // all the includes have been pre-procssed, hence most errors should have been caught
   // get the cached ECF_MICRO variable, typically its one char.
   string ecfMicro = ecfMicroCache_;
   std::vector<std::string> tokens;

   bool add = false;
   for (std::vector< std::string >::const_iterator i = lines.begin(); i!= lines.end(); ++i){
      if ( (*i).find(ecfMicro) == 0) {
         if ( (*i).find( T_MANUAL ) == 1 )     { add = true;  continue; }
         if ( add && (*i).find( T_END ) == 1 ) { add = false; continue; }

         if ((*i).find(T_ECFMICRO) == 1) {  // %ecfmicro #
            tokens.clear();
            Str::split( (*i), tokens );
            if (tokens.size() < 2) {
               std::stringstream ss; ss << "ecfmicro does not have a replacement character, in " << script_path_or_cmd_;
               errormsg += ss.str();
               return false;
            }
            ecfMicro = tokens[1];
            if (ecfMicro.size() > 2) {
               std::stringstream ss; ss << "Expected ecfmicro replacement to be a single character, but found '" << ecfMicro << "' " <<  ecfMicro.size() << " in file : " << script_path_or_cmd_;
               errormsg += ss.str();
               return false;
            }
            continue;
         }
      }
      if (add) { theManualLines.push_back(*i); }
   }
   if (add) {
      std::stringstream ss; ss << "Unterminated manual. Matching 'end' is missing, for file " << script_path_or_cmd_;
      errormsg += ss.str();
      dump_expanded_script_file(1,lines);
      return false;
   }
   return true;
}

std::string EcfFile::getIncludedFilePath( const std::string& includedFile,
         const std::string& line,
         std::string& errormsg)
{
   // Include can have following format:
   //   %include /tmp/file.name   -> /tmp/filename
   //   %include file.name        -> filename
   //   %include "../file.name"   -> script_directory_location/file.name
   //   %include "file.name"      -> %ECF_HOME%/%SUITE%/%FAMILY%/filename
   //   %include <file.name>      -> %ECF_INCLUDE%/filename
   //   When ECF_INCLUDE          -> path1:path2:path3
   //   %include <file.name>      -> path1/filename || path2/filename || path3/filename
   //
   //   %include <file.name>      -> ECF_HOME/filename

   std::string the_include_file = includedFile.substr( 1, includedFile.size() - 2 );
   if ( includedFile.size() >=2 && includedFile[1] == '/') {
      // filename starts with '/' no interpretation return as in
      // %include </home/ecf/fred.ecf>
      // %include "/home/ecf/fred.ecf"
      return the_include_file;
   }

   std::stringstream ss;
   if ( includedFile[0] == '<' ) {
      // %include <filename> can be one of:
      //    o When ECF_INCLUDE is a single path -> path1/filename
      //    o When ECF_INCLUDE is a multi  path -> path1:path2:path3
      //                                        -> path1/filename || path2/filename || path3/filename
      //    o ECF_HOME/filename
      std::string ecf_include;
      if (node_->findParentUserVariableValue( Str::ECF_INCLUDE() , ecf_include ) && !ecf_include.empty() ) {

         // if ECF_INCLUDE is a set a paths, search in order. i.e like $PATH
         if (ecf_include.find(':') != std::string::npos) {
            std::vector<std::string> include_paths;
            Str::split(ecf_include,include_paths,":");
            for(size_t i =0; i < include_paths.size();i++) {
               ecf_include.clear();
               ecf_include = include_paths[i];
               ecf_include += '/';
               ecf_include += the_include_file;

               // Don't rely on hard coded paths. Added for testing, but could be generally useful
               // since in test scenario ECF_INCLUDE is defined relative to $ECF_HOME
               node_->enviromentSubsitution(ecf_include);

               if (fs::exists(ecf_include)) return ecf_include;
            }
         }
         else {
            ecf_include += '/';
            ecf_include += the_include_file;
            node_->enviromentSubsitution(ecf_include);
            if (fs::exists(ecf_include)) return ecf_include;
         }

         // ECF_INCLUDE is specified *BUT* the file does *NOT* exist, Look in ECF_HOME
      }

      // WE get HERE *if* ECF_INCLUDE not specified, or if specified but file *not found*
      ecf_include.clear();
      node_->findParentVariableValue( Str::ECF_HOME() , ecf_include );
      if (ecf_include.empty()) {
         ss << "ECF_INCLUDE/ECF_HOME not specified, for task " << node_->absNodePath() << " at " << line;
         errormsg += ss.str();
         return string();
      }

      ecf_include += '/';
      ecf_include += the_include_file;

      return ecf_include;
   }
   else if ( includedFile[0] == '"' ) {

      // we have two forms: "head.h" & "../head.h"

      std::string path;
      if ( includedFile.find("../") == 1) {
         // Find the include file relative to the ecf file.  "../head.h"
         // remove the leading and trailing '"'
         std::string the_included_file = includedFile;
         Str::removeQuotes(the_included_file);

         // remove leading ../  ECFLOW-274
         Str::replace(the_included_file,"../","");

         // Get the root path, i.e. script_path_or_cmd_ is of the form "/user/home/ma/mao/course/t1.ecf"
         // we need "/user/home/ma/mao/course/"
         std::string::size_type last_slash = script_path_or_cmd_.rfind("/");
         if (last_slash != std::string::npos) {
            path = script_path_or_cmd_.substr( 0, last_slash + 1 );
            path += the_included_file;
            // std::cout << "path == " << path << "\n";
            return path;
         }
      }

      // include contents of %ECF_HOME%/%SUITE%/%FAMILY%/filename
      node_->findParentUserVariableValue( Str::ECF_HOME() , path);
      if ( path.empty() ) {
         ss << "ECF_HOME not specified, for task " << node_->absNodePath() << " at " << line;
         errormsg += ss.str();
         return string();
      }
      path += '/';
      std::string suite;
      node_->findParentVariableValue( "SUITE" , suite);   // SUITE is a generated variable
      if ( suite.empty() ) {
         ss << "SUITE not specified, for task " << node_->absNodePath() << " at " << line;
         errormsg += ss.str();
         return string();
      }
      path += suite;
      path += '/';
      std::string family;
      node_->findParentVariableValue( "FAMILY" , family); // FAMILY is a generated variable
      if ( family.empty() ) {
         ss << "FAMILY not specified, for task " << node_->absNodePath() << " at " << line;
         errormsg += ss.str();
         return string();
      }
      path += family;
      path += '/';
      path += the_include_file ; // "filename"
      return path;
   }

   // File either has an absolute pathname or is in the current working dir.
   // include file name as is, from current working directory
   return includedFile;
}

void EcfFile::removeCommentAndManual()
{
   // get the cached ECF_MICRO variable, typically its one char.
   string ecfMicro = ecfMicroCache_;

   // We need a stack to properly implement nopp. This is required since we need to pair
   // the %end, with nopp. i.e need to handle
   // %nopp
   // %comment
   // %end      // this is paired with comment
   // %end      // This is paired with nopp
   const int NOPP = 0;
   const int COMMENT = 1;
   const int MANUAL = 2;
   bool nopp = false;
   bool erase = false;
   std::vector<int> pp_stack;
   std::vector<std::string> tokens;

   for(std::vector<std::string>::iterator i=jobLines_.begin(); i!=jobLines_.end(); ++i) {

       // take into account micro char during  removal of comment/manual
       string::size_type ecfmicro_pos = (*i).find(ecfMicro);
       if (ecfmicro_pos == 0) {

          // We can not remove comments/manuals between %nopp/%end
          if ((*i).find(T_MANUAL)  == 1) {
             pp_stack.push_back(MANUAL);
             if (nopp) continue;

             // cerr << "EcfFile::removeCommentAndManual erase = " << erase << " " << *i << "\n";
             jobLines_.erase( i-- );  // remove  %manual
             if (erase) {
                std::stringstream ss; ss << "EcfFile::removeCommentAndManual: Embedded manuals are not allowed in " << script_path_or_cmd_;
                throw std::runtime_error( ss.str() );
             }
             erase = true;
             continue;
          }

          if ((*i).find(T_COMMENT) == 1) {
             pp_stack.push_back(COMMENT);
             if (nopp) continue;

             // cerr << "EcfFile::removeCommentAndManual erase = " << erase << " " << *i << "\n";
             jobLines_.erase( i-- ); // remove %comment
             if (erase) {
                std::stringstream ss; ss << "EcfFile::removeCommentAndManual: Embedded comments are not allowed in " << script_path_or_cmd_;
                throw std::runtime_error( ss.str() );
             }
             erase = true;
             continue;
          }

          if ((*i).find(T_NOOP) == 1) { pp_stack.push_back(NOPP); nopp = true; continue; }

          if ((*i).find(T_END) == 1) {
             if (pp_stack.empty()) throw std::runtime_error("EcfFile::removeCommentAndManual: failed  unpaired %end");
             int last_directive = pp_stack.back(); pp_stack.pop_back();
             if (last_directive == NOPP) nopp = false;
             else {
//                cerr << "EcfFile::removeCommentAndManual erase = " << erase << " " << *i << "\n";
                if (erase) {
                   jobLines_.erase( i-- ); // remove %end associated with %comment and %manual
                   erase = false;
                }
             }
             continue;
          }

          if (!nopp && (*i).find(T_ECFMICRO) == 1) {

             tokens.clear();
             Str::split( (*i), tokens );
             if (tokens.size() < 2) {
                std::stringstream ss; ss << "ecfmicro does not have a replacement character, in " << script_path_or_cmd_;
                throw std::runtime_error("EcfFile::removeCommentAndManual: failed " + ss.str());
             }
             ecfMicro = tokens[1];
          }
       }
       if ( nopp ) continue;

       //  remove all line between %comment and %end | %manual and %end
       if (erase) {
          jobLines_.erase( i-- );
       }
   }

   if (erase) {
      std::stringstream ss;
      ss << "Unterminated comment/manual. Matching 'end' is missing, in " << script_path_or_cmd_;
      throw std::runtime_error("EcfFile::removeCommentAndManual: failed " + ss.str());
   }
}

void EcfFile::remove_nopp_end_tokens()
{
   // get the cached ECF_MICRO variable, typically its one char.
   string ecfMicro = ecfMicroCache_;

   // We need a stack to properly implement nopp. This is required since we need to pair
   // the %end, with nopp. i.e need to handle
   // %nopp
   // %comment
   // %end      // this is paired with comment **** this should stay ****
   // %end      // This is paired with nopp    **** this should be deleted ****
   const int NOPP = 0;
   const int COMMENT = 1;
   const int MANUAL = 2;
   std::vector<int> pp_stack;
   std::vector< std::string > tokens;
   bool nopp = false;
   bool erase = false;

   for(std::vector<std::string>::iterator i=jobLines_.begin(); i!=jobLines_.end(); ++i) {

      string::size_type ecfmicro_pos = (*i).find(ecfMicro);
      if ( ecfmicro_pos == 0) {

          if ((*i).find(T_MANUAL)  == 1) { pp_stack.push_back(MANUAL); continue;  }
          if ((*i).find(T_COMMENT) == 1) { pp_stack.push_back(COMMENT); continue; }
          if ((*i).find(T_END) == 1) {
             if (pp_stack.empty()) throw std::runtime_error("EcfFile::remove_nopp_end_tokens: failed  unpaired %end");
             int last_directive = pp_stack.back(); pp_stack.pop_back();
             if (last_directive == NOPP) {
                nopp = false;
                jobLines_.erase( i-- );        // remove %end associated with %nopp
                erase = false;
             }
             continue;
          }
          if ((*i).find(T_NOOP) == 1) {
             pp_stack.push_back(NOPP); nopp = true;
             jobLines_.erase( i-- );      // remove %nopp
              if (erase) {
                 std::stringstream ss; ss << "Embedded nopp are not allowed " << script_path_or_cmd_;
                 throw std::runtime_error("EcfFile::remove_nopp_end_tokens: failed " + ss.str());
              }
              erase = true;
              continue;
          }
          if (!nopp && (*i).find(T_ECFMICRO) == 1) {  // %ecfmicro #

             tokens.clear();
             Str::split( *i, tokens );
             if (tokens.size() < 2) {
                std::stringstream ss;
                ss << "ecfmicro does not have a replacement character, in " << script_path_or_cmd_;
                throw std::runtime_error("EcfFile::remove_nopp_end_tokens: failed " + ss.str());
             }

             ecfMicro = tokens[1];         // override ecfMicro char
             jobLines_.erase( i-- );       // remove %ecfmicro &
             continue;
          }
      }
   }

   if (erase) {
      std::stringstream ss;
      ss << "Unterminated nopp. Matching 'end' is missing, in " << script_path_or_cmd_;
      throw std::runtime_error("EcfFile::remove_nopp_end_tokens: failed " + ss.str());
   }
}

int EcfFile::countEcfMicro(const std::string& line, const std::string& ecfMicro)
{
   if (line.find("#") != string::npos) {
      // ignore ecfmicro character in comments
      return 0;
   }

   /// Pound char could be more than one char ?
   int count = 0;
   if ( !ecfMicro.empty()) {
      const char theChar = ecfMicro[0];
      size_t end = line.size();
      for(size_t i = 0; i < end; ++i) {
         if (line[i] == theChar) {
            count++;
         }
      }
   }
   //	cerr << "line " << line << " count = " << count << "\n";
   return count;
}

void EcfFile::dump_expanded_script_file(size_t i, const std::vector<std::string>& lines)
{
#ifdef DEBUG_PRE_PROCESS
   if (i != 0) std::cout << "\nSee file tmp.ecf around line number " << i-1 << "\n";
   std::string err;
   if (!File::create("tmp" + get_extn(),lines,err))  std::cout << "Could not create file tmp.ecf\n";
#endif
}

/// returns the extension, i.e for task->.ecf for alias->.usr
const std::string& EcfFile::get_extn() const
{
   Submittable* task_or_alias = node_->isSubmittable();
   if (task_or_alias) return task_or_alias->script_extension();
   else {
      std::stringstream ss; ss << "EcfFile::get_extn(): Can only return extension for task/alias but found " << node_->debugNodePath();
      throw std::runtime_error(ss.str());
   }
   return Str::EMPTY();
}
