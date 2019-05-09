/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Openssl
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//  There are several ways of enabling SSL
//    1/ ECF_SSL,   ecflow_client,ecflow_server,ecflow.so,ecfluw_ui
//    2/ --ssl      ecflow_client,ecflow_server
//    3/ ecflow_ui  This can enable and disable ssl, even if enabled via environment ECF_SSL
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <stdlib.h>  // getenv
#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/bind.hpp>

#include "Openssl.hpp"
#include "File.hpp"
#include "Ecf.hpp"
#include "Host.hpp"
#include "Str.hpp"

using namespace std;
namespace fs = boost::filesystem;

namespace ecf {

std::string Openssl::info() const
{
   if (ssl_ == "1")  return "enabled : uses shared ssl certificates";
   return "enabled : uses server/port specific ssl certificates";
}

bool Openssl::enable_no_throw(std::string host,const std::string& port)
{
   if (host == Str::LOCALHOST())  host = Host().name();

   // Avoid disk access if possible if this function is called many times.
   if (!ssl_.empty()) {
      //cout << "Openssl::enable() Already called before ssl_ " << ssl_ << " ***************************************\n";
      if (ssl_ == "1") {
         // server.crt exist
         return true;
      }
      else {
         if (host_ == host && port_ == port) {
            // <host>.<port>.crt already exist
            return true;
         }

         // Carry on because host/port has changed.
      }
   }

//   if (Ecf::server()) cout << "Openssl::enable(SERVER) ---> input host:" << host << " port:" << port << "\n";
//   else               cout << "Openssl::enable(CLIENT) ---> input host:" << host << " port:" << port << "\n";

   // Look for the certificate that HAVE to exist on both client and server. i.e Self signed certificate (CRT)
   // Look for server.crt first. Done by setting ssl_ = "1";
   // Needed for testing, avoid <host>.<port>.crt when ports numbers are auto generated
   host_ = host;
   port_ = port;
   ssl_ = "1";
   if (!fs::exists(crt())) { // crt() uses ssl_

      // More specific per server. But we take an extra hit on file access.
      ssl_ = host;
      ssl_ += ".";
      ssl_ += port;
      if (!fs::exists(crt())) {  // crt() uses ssl_

         ssl_.clear();
         return false;
      }
   }
   //cout << "Openssl::enable input ssl_:'" << ssl_ << "'\n";
   return true;
}

void Openssl::enable(std::string host,const std::string& port)
{
   if (!enable_no_throw(host,port)) {
      std::stringstream ss;
      ss << "Openssl::enable: Error: Expected to find the self signed certificate file(CRT) server.crt or " << host << "." << port << ".crt in $HOME/.ecflowrc/ssl";
      throw std::runtime_error(ss.str());
   }
}

void Openssl::init_for_server()
{
//   std::cout << " Openssl::init_for_server  host :" << host_ << "@" << port_ << "\n";
//   std::cout << " Openssl::init_for_server ssl_:'" << ssl_ << "'\n";

   check_server_certificates();

   ssl_context_.set_options(
            boost::asio::ssl::context::default_workarounds
            | boost::asio::ssl::context::no_sslv2
            | boost::asio::ssl::context::single_dh_use);
   // this must be done before loading any keys. as below
   ssl_context_.set_password_callback(boost::bind(&Openssl::get_password, this));
   ssl_context_.use_certificate_chain_file( crt() );
   ssl_context_.use_private_key_file( key(), boost::asio::ssl::context::pem);
   ssl_context_.use_tmp_dh_file( pem() );
}

void Openssl::init_for_client()
{
//   std::cout << " Openssl::init_for_client host :" << host_ << "@" << port_ << "\n";
   if (!init_for_client_) {
      init_for_client_ = true;
      ssl_context_.load_verify_file(crt());
   }
}

std::string Openssl::get_password() const
{
   // std::cout << "Openssl::get_password()\n";
   std::string passwd_file = passwd();
   if (fs::exists(passwd_file)) {
      std::string contents;
      if (ecf::File::open(passwd_file,contents)) {
         // remove /n added by editor.
         if (!contents.empty() && contents[contents.size()-1] == '\n') contents.erase(contents.begin() + contents.size()-1);
         //std::cout << "Server::get_password() passwd('" << contents << "')\n";
         return contents;
      }
      else {
         std::stringstream ss;
         ss << "Server::get_password file " << passwd_file << " exists, but can't be opened (" << strerror(errno) << ")";
         throw std::runtime_error(ss.str());
      }
   }
   //std::cout << "Server::get_password() passwd('test')\n";
   return "test";
}


std::string Openssl::certificates_dir() const
{
   std::string home_path = getenv("HOME");
   home_path += "/.ecflowrc/ssl/";
   return home_path;
}

std::string Openssl::pem() const
{
   std::string str = certificates_dir();
   if (ssl_ == "1") { str += "dh1024.pem"; return str;}
   str += ssl_;
   str += ".pem";
   return str;
}

std::string Openssl::crt() const
{
   std::string str = certificates_dir();
   if (ssl_ == "1") { str += "server.crt"; return str;}
   str += ssl_;
   str += ".crt";
   return str;
}

std::string Openssl::key() const
{
   std::string str = certificates_dir();
   if (ssl_ == "1") { str += "server.key"; return str; }
   str += ssl_;
   str += ".key";
   return str;
}

std::string Openssl::passwd() const
{
   std::string str = certificates_dir();
   if (ssl_ == "1") { str += "server.passwd"; return str;}
   str += ssl_;
   str += ".passwd";
   return str;
}

const char* Openssl::ssl_info() {
   return
            "ecFlow client and server are SSL enabled. To use SSL choose between:\n"
            "  1. export ECF_SSL=1    # define an environment variable\n"
            "  2. use --ssl           # argument on ecflow_client/ecflow_server\n"
            "                         # Typically ssl server can be started with ecflow_start.sh -s\n"
            "  3. Client.enable_ssl() # for python client\n\n"
            "ecFlow expects the certificates to be in directory $HOME/.ecflowrc/ssl\n"
            "The certificates can be shared if you have multiple servers running on\n"
            "the same machine. To do this:\n"
            "ecflow_server expects the following files in $HOME/.ecflowrc/ssl\n"
            "   - dh1024.pem\n"
            "   - server.crt\n"
            "   - server.key\n"
            "   - server.passwd (optional) if this exists it must contain the pass phrase used to create server.key\n"
            "ecflow_client expects the following files in : $HOME/.ecflowrc/ssl\n"
            "   - server.crt (this must be the same as server)\n\n"
            "Alternatively you can have different setting for each server.\n"
            "Then server expect files of the type:\n"
            "   - <host>.<port>.pem\n"
            "   - <host>.<port>.crt\n"
            "   - <host>.<port>.key\n"
            "   - <host>.<port>.passwd (optional)\n"
            "and client expect files of the type:\n"
            "   - <host>.<port>.crt  # as before this must be same as the server\n"
            "The server/client will automatically check existence of both variants.\n"
            "but will give preference to NON <host>.<port>.*** variants first\n"
            "The following steps, show you how to create the certificate files.\n"
            "This may need to be adapted if you want to use <host>.<port>.***\n"
            "- Generate a password protected private key. This will request a pass phrase.\n"
            "  This key is a 1024 bit RSA key which is encrypted using Triple-DES and stored\n"
            "  in a PEM format so that it is readable as ASCII text\n"
            "     > openssl genrsa -des3 -out server.key 1024   # Password protected private key\n"
            "- Additional security.\n"
            "  If you want additional security, create a file called 'server.passwd' and add the pass phrase to the file.\n"
            "  Then set the file permission so that file is only readable by the server process.\n"
            "  Or you can choose to remove password requirement. In that case we don't need server.passwd file.\n"
            "     > cp server.key server.key.secure\n"
            "     > openssl rsa -in server.key.secure -out server.key  # remove password requirement\n"
            "- Sign certificate with private key (self signed certificate).Generate Certificate Signing Request(CSR).\n"
            "  This will prompt with a number of questions.\n"
            "  However please ensure 'common name' matches the host where your server is going to run.\n"
            "     > openssl req -new -key server.key -out server.csr # Generate Certificate Signing Request(CSR)\n"
            "- Generate a self signed certificate CRT, by using the CSR and private key.\n"
            "     > openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt\n"
            "- Generate dhparam file. ecFlow expects 1024 key.\n"
            "     > openssl dhparam -out dh1024.pem 1024"
            ;
}

void Openssl::check_server_certificates() const
{
//   cout << "Openssl::check_server_certificates: ssl'" << ssl_ << "'\n";

   {
      string server_key = key();
      if (!fs::exists(server_key)) throw std::runtime_error("Error: The password protected private server key file '" + server_key  + "' does not exist\n\n" + ssl_info() );
   }
   {
      string server_pem = pem();
      if (!fs::exists(server_pem)) throw std::runtime_error("Error: The dhparam file(pem) '" +  server_pem  + "' does not exist\n\n" + ssl_info());
   }
}

std::ostream& operator<<(std::ostream& os, const ecf::Openssl& ssl)
{
   ssl.print(os);
   return os;
}

}
