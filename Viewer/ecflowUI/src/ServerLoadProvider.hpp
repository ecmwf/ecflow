//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLOADPROVIDER_HPP
#define SERVERLOADPROVIDER_HPP

#include "VInfo.hpp"
#include "InfoProvider.hpp"

class FileWatcher;

class ServerLoadProvider : public InfoProvider
{
public:
   ServerLoadProvider(InfoPresenter* owner);

   void visit(VInfoServer*);
   void clear();
   void setAutoUpdate(bool);

private:
   void fetchFile();
   void fetchFile(ServerHandler *server,const std::string& fileName);
};

#endif // SERVERLOADPROVIDER_HPP
