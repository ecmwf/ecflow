#ifndef LOG_HPP_
#define LOG_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Log
// Author      : Avi
// Revision    : $Revision: #31 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Simple singleton implementation of log
//
// Please note how do we guarantee that file is actually written to disk:
// a/ Each log entry should call std::endl
// b/ Call flush() on the ofstream
// c/ The two methods will force ofstream buffer to be written to disk
//    Well NOT REALLY dues to file caching by the OS.
// The strongest hint we can give th OS to actually write to the physical medium
// is to close the file. (This does not guarantee it, but is the closest we can achieve)
//
// Why is this an issue ? Testing on cross platform HPUX/linux, requires that
// testing has access to the ECF log file. Initially the log file held an
// ofstream of log file as a data member. However this meant that flushing
// did not guarantee writing ECF log file to disk. Testing requires that we
// are able to clear and copy the log file for comparison.
// Hence we use another level of indirection, so that we able to close the
// log file, and hence can ensure that it gets written to disk
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <boost/lambda/lambda.hpp>

namespace ecf {

class LogImpl;

class Log {
public:
   enum LogType { MSG, LOG, ERR, WAR, DBG, OTH };
   static void create(const std::string& filename);
   static void destroy();
   static Log* instance() { return instance_;}

   /// If file is closed will open it
   /// Outputs t the file a message of type XXX:[HH:MM:SS D.M.YYYY] message
   /// where XXX is one of the LogType
   ///
   /// If the message has multiple newlines these are split
   /// LogType ERR,WAR,DBG will create a time stamp, otherwise the last cached
   /// time stamp is used.
   bool log(LogType,const std::string& message);

   /// Single line message is placed in log file without a newline
   bool log_no_newline(LogType,const std::string& message);

   /// Append to log file file and add newline
   bool append(const std::string& message);

   /// Set the time stamp once for each request.
   void cache_time_stamp();

   /// Return the last cached time stamp. Used for time stamping edit history
   const std::string& get_cached_time_stamp() const;

   /// returns the contents of the log file, or the last n lines
   /// Will throw an std::runtime_error if the log file can not be opened
   /// Will close the file.
   static int get_last_n_lines_default() { return 100;}
   std::string contents(int get_last_n_lines);

   /// Will call flush and close the file. See notes above
   void flush();

   /// will flush the log file without closing it.
   void flush_only();

   /// clear the log file. Required for testing
   void clear();

   /// Close the existing log file, and new start writing to the new location
   void new_path(const std::string& the_new_path);

   /// Returns the current log file path name
   std::string path() const;

   // returns vec = MSG, LOG, ERR, WAR, DBG, OTH
   static void get_log_types(std::vector<std::string>&);

private:

   /// make sure path is not a directory & path has a parent directory.
   /// Will throw std::runtime_error for errors
   static void check_new_path(const std::string& new_path);

private:
  Log(const Log&) = delete;
  const Log& operator=(const Log&) = delete;

private:
   ~Log();
   explicit Log(const std::string& fileName);
   static Log* instance_;

   std::string fileName_;
   LogImpl* logImpl_;
};

// Flush log on destruction
class LogFlusher {
public:
   LogFlusher(){}
   ~LogFlusher();
private:
   LogFlusher(const  LogFlusher&) = delete;
   const  LogFlusher& operator=(const LogFlusher&) = delete;
};

/// The LogImpl allows the ofstream object to be closed, and so provide strongest
/// hint to OS that we want file to be written to physical medium.
/// This is required for testing purposes, as each test run needs to clear/copy
/// the log file
class LogImpl {
public:
   LogImpl(const std::string& filename);
   ~LogImpl();

   bool log(Log::LogType lt,const std::string& message) { return do_log(lt,message,true); }
   bool log_no_newline(Log::LogType lt,const std::string& message) { return do_log(lt,message,false); }
   bool append(const std::string& message);

   void create_time_stamp();
   const std::string& get_cached_time_stamp() const { return time_stamp_;}

   void flush();

private:
   bool do_log(Log::LogType,const std::string& message, bool newline);
   bool check_file_write(const std::string& message) const;

private:
  LogImpl(const LogImpl&) = delete;
  const LogImpl& operator=(const LogImpl&) = delete;

private:
   unsigned int count_;
   std::string time_stamp_;
   mutable std::ofstream file_;

   std::string log_type_and_time_stamp_; // re-use memory
};

/// Utility class used for debug. Enables log file messages to be written to standard out
class LogToCout {
public:
   LogToCout()   { flag_ = true;}
   ~LogToCout()  { flag_ = false;}
   static bool ok() { return flag_;}
private:
   LogToCout(const LogToCout&) = delete;
   const LogToCout& operator=(const LogToCout&) = delete;
   static bool flag_;
};

bool log(Log::LogType,const std::string& message);
bool log_no_newline(Log::LogType,const std::string& message);
bool log_append(const std::string& message);
void log_assert(char const* expr,char const* file, long line, const std::string& message);

// allow user to do the following:
// LOG(Log::WAR,"this is " << path << " ok ");
//
// helper, see STRINGIZE() macro
template <typename Functor>
std::string stringize_f(Functor const & f) {
   std::ostringstream out;
   f(out);
   return out.str();
}
#define STRINGIZE(EXPRESSION)  (ecf::stringize_f(boost::lambda::_1 << EXPRESSION))
#define LOG(level, EXPRESSION) ecf::log(level, STRINGIZE(EXPRESSION))
#define LOG_ASSERT(expr,EXPRESSION) ((expr)? ((void)0): ecf::log_assert(#expr, __FILE__, __LINE__, STRINGIZE(EXPRESSION)))

}

#endif
