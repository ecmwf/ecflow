#ifndef HOST_HPP_
#define HOST_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
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

#include <boost/noncopyable.hpp>
#include <string>

namespace ecf {

class Host : private boost::noncopyable {
public:
	/// can throw std::runtime_error if the gethostname fails
   Host();
   Host(const std::string& host);

	/// return the host name
	std::string name() const;

	/// returns the log file name
   std::string ecf_log_file(const std::string& port) const;

	/// return checkPoint file
   std::string ecf_checkpt_file(const std::string& port) const;

	/// return backup checkPoint file
   std::string ecf_backup_checkpt_file(const std::string& port) const;

   /// return ecf.list file. White list file used for authentication & authorisation
   std::string ecf_lists_file(const std::string& port) const;

   /// return ecf.passwd file. Used for authentication
   std::string ecf_passwd_file(const std::string& port) const;

   /// Given a port and file name, will return <host>.<port>.file_name
   std::string prefix_host_and_port( const std::string& port,const std::string& list_file ) const;

private:
	std::string host_port_prefix(const std::string& port) const;
	void get_host_name(); // will cache host name, to avoid multiple sysm calls
	std::string the_host_name_;
};
}
#endif
