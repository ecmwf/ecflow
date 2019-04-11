/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
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
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Openssl.hpp"
#include <stdlib.h>  // getenv

using namespace std;
namespace fs = boost::filesystem;

namespace ecf {

std::string Openssl::certificates_dir()
{
   std::string home_path = getenv("HOME");
   home_path += "/.ecflowrc/ssl/";
   return home_path;
}

const char* Openssl::ssl_info() {
   return
            "ecFlow client and server are SSL enabled. This can be done in 2 ways, choose one:\n"
            "   1. export ECF_SSL=1 # define an environment variable\n"
            "   2. use --ssl argument on ecflow_client/ecflow_server\n"
            "      Typically ssl server can be started with ecflow_start.sh -s\n\n"
            "ecflow_server expects the following files in : $HOME/.ecflowrc/ssl\n"
            "   - dh1024.pem\n"
            "   - server.crt\n"
            "   - server.key\n"
            "   - server.passwd (optional) if this exists it must contain the pass phrase used to create server.key\n\n"
            "ecflow_client expects the following files in : $HOME/.ecflowrc/ssl\n"
            "   - server.crt (this must be the same as server)\n\n"
            "The following steps, show you how to create the certificate files.\n"
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

void Openssl::check_server_certificates()
{
   std::string cert_dir = Openssl::certificates_dir();
   if (!fs::exists(cert_dir)) throw std::runtime_error("Error: The certificates directory '" + cert_dir + "' does not exist\n\n" + ssl_info());

   {
      string server_key = cert_dir + "server.key";
      if (!fs::exists(server_key)) throw std::runtime_error("Error: The password protected private server key file '" + server_key  + "' does not exist\n\n" + ssl_info() );
   }
   {
      string server_crt = cert_dir + "server.crt";
      if (!fs::exists(server_crt)) throw std::runtime_error("Error: The self signed certificate file(CRT) '" + server_crt  + "' does not exist\n\n" + ssl_info());
   }
   {
      string pem = cert_dir + "dh1024.pem";
      if (!fs::exists(pem)) throw std::runtime_error("Error: The dhparam file(pem) '" +  pem  + "' does not exist\n\n" + ssl_info());
   }
}

void Openssl::check_client_certificates()
{
   std::string cert_dir = Openssl::certificates_dir();
   if (!fs::exists(cert_dir)) throw std::runtime_error("Error: The certificates directory '" + cert_dir + "' does not exist\n\n" + ssl_info());
   {
      string server_crt = cert_dir + "server.crt";
      if (!fs::exists(server_crt)) throw std::runtime_error("Error: The self signed certificate file(CRT) '" + server_crt  + "' does not exist\n\n" + ssl_info());
   }
}

}
