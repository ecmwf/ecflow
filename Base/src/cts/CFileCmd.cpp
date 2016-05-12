/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #42 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <sstream>
#include "boost/filesystem/operations.hpp"
#include <boost/lexical_cast.hpp>

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "File.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Task.hpp"
#include "EcfFile.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

CFileCmd::CFileCmd(const std::string& pathToNode, const std::string& file_type, const std::string& input_max_lines)
: file_(ECF), pathToNode_(pathToNode), max_lines_(File::MAX_LINES())
{
   // std::cout << "CFileCmd::CFileCmd the_max_lines " << the_max_lines << "\n";

   if (file_type == "script") file_ = CFileCmd::ECF;
   else if (file_type == "job") file_ = CFileCmd::JOB;
   else if (file_type == "jobout") file_ = CFileCmd::JOBOUT;
   else if (file_type == "manual") file_ = CFileCmd::MANUAL;
   else if (file_type == "kill") file_ = CFileCmd::KILL;
   else if (file_type == "stat") file_ = CFileCmd::STAT;
   else {
      std::stringstream ss;
      ss << "CFileCmd::CFileCmd: Unrecognised file type " << file_type << " expected one of [script | job | jobout | manual | kill | stat] \n";
      throw std::runtime_error( ss.str() );
   }

   if (!input_max_lines.empty()){
      try {
         // Note: max_lines_ if type size_t, hence we cast to int to check for negative numbers
         int the_max_lines =  boost::lexical_cast<int>( input_max_lines );
         if (the_max_lines <= 0)  the_max_lines = File::MAX_LINES();
         max_lines_ = the_max_lines;
      }
      catch ( boost::bad_lexical_cast& e) {
         std::stringstream ss;
         ss << "CFileCmd::CFileCmd: The third argument(" << input_max_lines << ") must be convertible to an integer\n";
         throw std::runtime_error(ss.str());
      }
   }
}


std::vector<CFileCmd::File_t>  CFileCmd::fileTypesVec()
{
	std::vector<CFileCmd::File_t> vec; vec.reserve(5);
	vec.push_back(CFileCmd::ECF);
	vec.push_back(CFileCmd::JOB);
	vec.push_back(CFileCmd::JOBOUT);
   vec.push_back(CFileCmd::MANUAL);
   vec.push_back(CFileCmd::KILL);
   vec.push_back(CFileCmd::STAT);
	return vec;
}

std::string CFileCmd::toString(CFileCmd::File_t ft)
{
	switch (ft) {
		case CFileCmd::ECF:    return "script"; break;
		case CFileCmd::MANUAL: return "manual"; break;
		case CFileCmd::JOB:    return "job"; break;
      case CFileCmd::JOBOUT: return "jobout"; break;
      case CFileCmd::KILL:   return "kill"; break;
      case CFileCmd::STAT:   return "stat"; break;
		default : break;
	}
	return "script";
}

bool CFileCmd::equals(ClientToServerCmd* rhs) const
{
	CFileCmd* the_rhs = dynamic_cast<CFileCmd*>(rhs);
	if (!the_rhs)  return false;
	if ( file_       != the_rhs->fileType())   { return false; }
	if ( max_lines_  != the_rhs->max_lines())  { return false; }
	if ( pathToNode_ != the_rhs->pathToNode()) { return false; }
	return UserCmd::equals(rhs);
}

std::ostream& CFileCmd::print(std::ostream& os) const
{
	return user_cmd(os,CtsApi::to_string(CtsApi::file(pathToNode_,toString(file_),boost::lexical_cast<std::string>(max_lines_))));
}

STC_Cmd_ptr CFileCmd::doHandleRequest(AbstractServer* as) const
{
	// cout << "CFileCmd::doHandleRequest " << toString(file_) << " =======================================\n";
	switch (file_) {
		case CFileCmd::ECF:    as->update_stats().file_ecf_++;    break;
		case CFileCmd::MANUAL: as->update_stats().file_manual_++; break;
		case CFileCmd::JOB:    as->update_stats().file_job_++;    break;
      case CFileCmd::JOBOUT: as->update_stats().file_jobout_++; break;
      case CFileCmd::KILL:   as->update_stats().file_cmdout_++; break;
      case CFileCmd::STAT:   as->update_stats().file_cmdout_++; break;
	}

 	node_ptr node = find_node(as,pathToNode_); // will throw if defs not defined, or node not found

	std::string fileContents;
	Submittable* submittable = node->isSubmittable();
	if ( submittable ) {

 		switch (file_) {
			case CFileCmd::ECF: {
 				EcfFile ecf_file = submittable->locatedEcfFile(); // will throw std::runtime_error for errors
				ecf_file.script(fileContents);                    // will throw std::runtime_error for errors
				break;}

			case CFileCmd::MANUAL: {
				EcfFile ecf_file = submittable->locatedEcfFile(); // will throw std::runtime_error for errors
 				ecf_file.manual(fileContents);                    // will throw std::runtime_error for errors
 				break;}

			case CFileCmd::JOB: {
		      std::string ecf_job_file;
		      submittable->findParentVariableValue(Str::ECF_JOB(), ecf_job_file);
				if (!File::open(ecf_job_file,fileContents)) {
					std::stringstream ss;
					ss << "CFileCmd::doHandleRequest: Failed to open the job file('" << ecf_job_file << "') for task " << pathToNode_;
					throw std::runtime_error( ss.str() ) ;
 				}
				break;}

			case CFileCmd::JOBOUT: {

			   // ECF_JOBOUT is either constructed from ECF_OUT *OR* ECF_HOME/ECF_NAME.ECF_TRYNO
			   // See: Submittable.cpp: SubGenVariables::update_generated_variables()
			   //
			   // Typically if ECF_OUT is specified, we should only look at that location
			   // however SMS also looked at the alternate location (and RD relied on this ECFLOW-177 )

				const Variable& ecf_jobout_gen_var = submittable->findGenVariable(Str::ECF_JOBOUT());
				if (!File::open(ecf_jobout_gen_var.theValue(),fileContents)) {

				   // If that fails as a backup, look under ECF_HOME/ECF_NAME.ECF_TRYNO,   ECFLOW-177 preserve old SMS behaviour
				   std::string backup_jobout;
				   submittable->findParentUserVariableValue(Str::ECF_HOME(), backup_jobout);
				   backup_jobout += submittable->absNodePath();
				   backup_jobout += ".";
				   backup_jobout += submittable->tryNo();

				   if ( backup_jobout != ecf_jobout_gen_var.theValue()) {
				      // Implies ECF_OUT was specified, hence *ALSO* look in ECF_HOME/ECF_NAME.ECF_TRYNO
				      if (!File::open(backup_jobout,fileContents)) {
				         std::stringstream ss;
				         ss << "CFileCmd::doHandleRequest: Failed to open the job-out(ECF_JOBOUT=ECF_OUT/ECF_NAME.ECF_TRYNO) file('"
				               << ecf_jobout_gen_var.theValue() << "') *AND* at location (ECF_JOBOUT=ECF_HOME/ECF_NAME.ECF_TRYNO)('"
				               << backup_jobout << "') for task " << pathToNode_;
				         throw std::runtime_error( ss.str() ) ;
				      }
				   }
				   else {
				      // ECF_OUT *not* specified implies ECF_JOBOUT = ECF_HOME/ECF_NAME.ECF_TRYNO
                  std::stringstream ss;
                  ss << "CFileCmd::doHandleRequest: Failed to open the job-out(ECF_JOBOUT=ECF_HOME/ECF_NAME.ECF_TRYNO) file('"
                        << ecf_jobout_gen_var.theValue() << "') for task " << pathToNode_;
                  throw std::runtime_error( ss.str() ) ;
				   }
				}

				break; }

         case CFileCmd::KILL: {
            std::string ecf_job_file;
            submittable->findParentVariableValue(Str::ECF_JOB(), ecf_job_file);
            std::string file = ecf_job_file + ".kill";
            if (!File::open(file,fileContents)) {
               std::stringstream ss;
               ss << "CFileCmd::doHandleRequest: Failed to open the kill output file('" << file << "') for task " << pathToNode_;
               throw std::runtime_error( ss.str() ) ;
            }
            break; }

         case CFileCmd::STAT: {
            std::string ecf_job_file;
            submittable->findParentVariableValue(Str::ECF_JOB(), ecf_job_file);
            std::string file = ecf_job_file + ".stat";
            if (!File::open(file,fileContents)) {
               std::stringstream ss;
               ss << "CFileCmd::doHandleRequest: Failed to open the status output file('" << file << "') for task " << pathToNode_;
               throw std::runtime_error( ss.str() ) ;
            }
            break; }

			default : assert(false); break;
 		}
	}
	else {

		if (file_ == CFileCmd::MANUAL) {
			// The only valid option for Suite or Family is Manual

		   // First look for .man files in ECF_FILES and then ECF_HOME
         std::string ecf_files;
         node->findParentUserVariableValue( Str::ECF_FILES(), ecf_files);
         if ( !ecf_files.empty() && fs::exists( ecf_files ) && fs::is_directory( ecf_files ) ) {

            std::string manFile = File::backwardSearch( ecf_files, node->absNodePath(), File::MAN_EXTN());
            if (!manFile.empty()) {
               EcfFile the_file(node.get(), manFile);
               the_file.manual(fileContents); // pre-process & extract manual: will throw std::runtime_error for errors
            }
         }

         if ( fileContents.empty() ) {
            // Try under ECF_HOME
            std::string ecf_home;
            node->findParentUserVariableValue( Str::ECF_HOME(), ecf_home);
            if ( !ecf_home.empty() && fs::exists( ecf_home ) && fs::is_directory( ecf_home ) ) {

               std::string manFile = File::backwardSearch( ecf_home, node->absNodePath(), File::MAN_EXTN());
               EcfFile the_file(node.get(), manFile);
               the_file.manual(fileContents); // pre-process & extract manual: will throw std::runtime_error for errors
            }
            else {
               std::string errorMsg = "Failed to find the manual for Suite/Family  "; errorMsg += pathToNode_;
               errorMsg += " ECF_HOME directory does not exist and/or ECF_FILES not defined or directory does not exist.";
               throw std::runtime_error( errorMsg ) ;
            }
         }
		}
		else {
			std::stringstream ss; ss << "Option " << CFileCmd::toString(file_) << " is only valid for tasks. ";
			throw std::runtime_error( ss.str() ) ;
  		}
	}

	/// The file could get very large, hence truncate at the start
	if (Str::truncate_at_start(fileContents, max_lines_)) {
		std::stringstream ss;
		ss << "\n# >>>>>>>> File truncated down to " << max_lines_ << ". Truncated from the end of the file <<<<<<<<<\n";
		fileContents += ss.str();
	}

	return PreAllocatedReply::string_cmd(fileContents);
}

const char* CFileCmd::arg()  { return CtsApi::fileArg();}
const char* CFileCmd::desc() {
	        /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   return   "Return the chosen file. Select from [ script<default> | job | jobout | manual | kill | stat ]\n"
            "By default will return the script.\n"
            "  arg1 = path to node\n"
            "  arg2 = (optional) [ script<default> | job | jobout | manual | kill | stat ]\n"
            "         kill will attempt to return output of ECF_KILL_CMD, i.e the file %ECF_JOB%.kill\n"
            "         stat will attempt to return output of ECF_STATUS_CMD, i.e the file %ECF_JOB%.stat\n"
            "  arg3 = (optional) max_lines = 10000 <default>"
            ;
}

void CFileCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( CFileCmd::arg(),po::value< vector<string> >()->multitoken(), CFileCmd::desc() );
}
void CFileCmd::create( 	Cmd_ptr& cmd,
						boost::program_options::variables_map& vm,
						AbstractClientEnv* ac) const
{
	vector<string> args = vm[  arg() ].as< vector<string> >();

	if (ac->debug()) dumpVecArgs(CFileCmd::arg(),args);

	if (args.size() < 1 ) {
		std::stringstream ss;
		ss << "CFileCmd: At least one arguments expected for File. Found " << args.size() << "\n" << CFileCmd::desc() << "\n";
		throw std::runtime_error( ss.str() );
	}

	std::string pathToNode = args[0];

	std::string file_type = "script";
	if (args.size() >= 2) {
	   file_type = args[1];
	}

	std::string max_lines;
	if (args.size() == 3) {
	   max_lines = args[2];
	}

	cmd = Cmd_ptr( new CFileCmd(pathToNode, file_type, max_lines) );
}

std::ostream& operator<<(std::ostream& os, const CFileCmd& c) { return c.print(os); }

