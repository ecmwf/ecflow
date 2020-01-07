//============================================================================
// Name        : Log
// Author      : Avi
// Revision    : $Revision: #57 $ 
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Simple singleton implementation of log
//============================================================================
#include <assert.h>
#include <vector>
#include <iostream>

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

#include "Log.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "Indentor.hpp"
#include "TimeStamp.hpp"

using namespace std;
namespace fs = boost::filesystem;

namespace ecf {

Log* Log::instance_ = NULL;
bool LogToCout::flag_ = false;

void Log::create(const std::string& filename)
{
	if ( instance_ == NULL) {
		instance_ = new Log(filename);
	}
}

void Log::destroy()
{
	if (instance_) instance_->flush();

	delete instance_;
	instance_ = NULL;
}

Log::Log(const std::string& fileName)
: fileName_(fileName), logImpl_( new LogImpl(fileName) )
{
}

void Log::create_logimpl()
{
   if (!logImpl_) {
      logImpl_ = new LogImpl(fileName_);
   }
}

Log::~Log()
{
	delete logImpl_;
	logImpl_ = NULL;
}

bool Log::log(Log::LogType lt,const std::string& message)
{
   create_logimpl();

	if (! logImpl_->log(lt,message)) {
	   // handle write failure and Get the failure reason. This will delete logImpl_ & recreate
      std::string fail_message = handle_write_failure();

      (void)logImpl_->log(Log::ERR,fail_message); // must be after handle_write_failure
	   (void)logImpl_->log(lt,message);
	   return false;
	}
	return true;
}

bool Log::log_no_newline(Log::LogType lt,const std::string& message)
{
   create_logimpl();

   if (! logImpl_->log_no_newline(lt,message)) {
      // handle write failure and Get the failure reason. This will delete logImpl_ & recreate
      std::string fail_message = handle_write_failure();

      (void)logImpl_->log(Log::ERR,fail_message);  // must be after handle_write_failure
      (void)logImpl_->log_no_newline(lt,message);
      return false;
   }
   return true;
}

bool Log::append(const std::string& message)
{
   create_logimpl();

   if (! logImpl_->append(message)) {
      // handle write failure and Get the failure reason. This will delete logImpl_ & recreate
      std::string fail_message = handle_write_failure();

      (void)logImpl_->log(Log::ERR,fail_message);  // must be after handle_write_failure
      (void)logImpl_->append(message);
      return false;
   }
   return true;
}

void Log::cache_time_stamp()
{
   create_logimpl();
	logImpl_->create_time_stamp();
}

const std::string& Log::get_cached_time_stamp() const
{
   if (!logImpl_)  {
      return Str::EMPTY();
   }
   return logImpl_->get_cached_time_stamp();
}

void Log::flush()
{
 	// will close ofstream and force data to be written to disk.
	// Forcing writing to physical medium can't be guaranteed though!
	delete logImpl_;
	logImpl_ = NULL;
}

void Log::flush_only()
{
   if (logImpl_) logImpl_->flush();
}

void Log::clear()
{
	flush();

	// Open and truncate the file.
 	std::ofstream logfile(fileName_.c_str(), ios::out | ios::trunc);
 	if (logfile.is_open()) {
 		logfile.close(); // force buffers to flush
 	}
}

void Log::new_path(const std::string& the_new_path)
{
   check_new_path(the_new_path);

	// flush and close log file
	flush();

	fileName_ = the_new_path;
}

void Log::check_new_path(const std::string& new_path)
{
   if (new_path.empty()) {
      throw std::runtime_error("Log::check_new_path: No path name specified for the new log file");
   }

   fs::path the_new_path = new_path;

   // Allow paths like "fred.log"
   fs::path parent_path = the_new_path.parent_path();
   //std::cout << "the_new_path.parent_path() = " << parent_path << "\n";
   if (!parent_path.empty()) {

      if (!fs::exists(parent_path)) {
         std::stringstream ss;
         ss << "Log::check_new_path: Can not create new log file, since the directory part " << parent_path << " does not exist\n";
         throw std::runtime_error(ss.str());
      }
   }

   // Now check that path does not correspond to a directory, can't use that as the new log file location
   if (fs::is_directory(the_new_path)) {
      std::stringstream ss;
      ss << "LogCmd::LogCmd: Can not create new log file, since the path correspond to a directory " << the_new_path << "\n";
      throw std::runtime_error(ss.str());
   }
}

std::string Log::path() const
{
   if (!fileName_.empty() && fileName_[0] == '/') {
      // Path is absolute return as is
      return fileName_;
   }
   std::string the_path = fs::current_path().string();
   the_path += "/";
   the_path += fileName_;
   return the_path;
}

std::string Log::contents(int get_last_n_lines)
{
   if ( get_last_n_lines == 0 ) {
      return string();
   }

   // Close the file. Log file may be buffered, hence flush first
   flush();

   std::string error_msg;
   if (get_last_n_lines > 0 ) {
      return File::get_last_n_lines(fileName_,get_last_n_lines,error_msg);
   }
   return File::get_first_n_lines(fileName_,std::abs(get_last_n_lines),error_msg);
}

std::string Log::handle_write_failure()
{
   std::string msg = "Failed to write to log file: ";
   msg += File::stream_error_condition(logImpl_->stream());
   msg += " : Attempting to close/reopen log file";

   if (LogToCout::ok()) Indentor::indent(cout) << msg << '\n';

   // handle write failure, by closing then re-opening log file
   delete logImpl_; logImpl_ =  NULL;
   create_logimpl();

   return msg;
}



bool log(Log::LogType lt,const std::string& message)
{
	if (Log::instance()) {
 		return Log::instance()->log(lt,message);
	}
	else {
		if (LogToCout::ok()) {
			Indentor::indent(cout) << message << '\n';
		}
	}
	return true;
}

bool log_no_newline(Log::LogType lt,const std::string& message)
{
   if (Log::instance()) {
      return Log::instance()->log_no_newline(lt,message);
   }
   else {
      if (LogToCout::ok()) {
         Indentor::indent(cout) << message << '\n';
      }
   }
	return true;
}

bool log_append(const std::string& message)
{
   if (Log::instance()) {
      return Log::instance()->append(message);
   }
   else {
      if (LogToCout::ok()) {
         Indentor::indent(cout) << message << '\n';
      }
   }
   return true;
}

void log_assert(char const* expr,char const* file, long line, const std::string& message)
{
	std::stringstream ss;
	ss << "ASSERT failure: " << expr << " at " << file << ":" << line << " " << message;
	std::string assert_msg = ss.str();
	cerr << assert_msg << "\n";
	if (Log::instance()) {
		Log::instance()->log(Log::ERR,assert_msg);
 		exit(1);
  	}
}

// returns vec = MSG, LOG, ERR, WAR, DBG, OTH
void Log::get_log_types(std::vector<std::string>& vec)
{
   vec.reserve(6);
   vec.push_back("MSG");
   vec.push_back("LOG");
   vec.push_back("ERR");
   vec.push_back("WAR");
   vec.push_back("DBG");
   vec.push_back("OTH");
}

//======================================================================================================

LogFlusher::~LogFlusher()
{
   Log* the_log = Log::instance();
   if ( the_log ) {
      the_log->flush_only(); // flush without closing log file.
   }
}


//======================================================================================================
LogImpl::LogImpl(const std::string& filename)
: count_(0), file_(filename.c_str(), ios::out | ios::app)
{
 	if (!file_.is_open()) {
 	   std::string msg = "LogImpl::LogImpl: Could not open log file '";
 	   msg += filename;
 	   msg += "' ";
 	   msg += File::stream_error_condition(file_);
		std::cerr << msg << "\n";
		// Do *NOT* throw std::runtime_error(msg), HERE as this can cause server to die.
		// This would diagnose ECFLOW-1558 but is not acceptable
	}
}

LogImpl::~LogImpl() {}


static void append_log_type(std::string& str, Log::LogType lt)
{
   switch (lt) {
      case Log::MSG: str.append("MSG:"); break;
      case Log::LOG: str.append("LOG:"); break;
      case Log::ERR: str.append("ERR:"); break;
      case Log::WAR: str.append("WAR:"); break;
      case Log::DBG: str.append("DBG:"); break;
      case Log::OTH: str.append("OTH:"); break;
      default: assert(false); break;
   }
}

bool LogImpl::do_log(Log::LogType lt,const std::string& message, bool newline)
{
   count_++;

   // XXX:[HH:MM:SS D.M.YYYY] chd:fullname [+additional information]
   // XXX:[HH:MM:SS D.M.YYYY] --<user_cmd> [+additional information]
   if (time_stamp_.empty() || lt == Log::ERR || lt == Log::WAR || lt == Log::DBG ) create_time_stamp();

   // re-use memory allocated to log_type_and_time_stamp_
   log_type_and_time_stamp_.clear();
   append_log_type(log_type_and_time_stamp_,lt);
   log_type_and_time_stamp_ += time_stamp_;

   if (message.find("\n") == std::string::npos) {
      file_ << log_type_and_time_stamp_ << message;
      if (newline) file_ << '\n';
   }
   else {
      // If message has \n then split into multiple lines
      std::vector< std::string > lines;
      Str::split(message,lines,"\n");
      size_t theSize = lines.size();
      for(size_t i = 0; i < theSize; ++i) {
         file_ << log_type_and_time_stamp_ << lines[i] << '\n';
      }
   }
   return file_.good();
}

void LogImpl::flush() {
   // cout << "LogImpl::flush() count = " << count_ << endl;
   if (count_ != 0) {
      file_.flush();
      count_ = 0;
   }
}

bool LogImpl::append(const std::string& message)
{
   count_++;
   file_ << message << '\n';
   return file_.good();
}

void LogImpl::create_time_stamp()
{
   TimeStamp::now(time_stamp_);
}

}
