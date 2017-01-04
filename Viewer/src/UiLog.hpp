//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef UILOG_HPP
#define UILOG_HPP

#include <string>
#include <sstream>

class QString;
class QModelIndex;

class ServerHandler;

class UiLog
{
public:
   enum Type {INFO,WARN,ERROR,DBG};

   UiLog() : type_(INFO) {}
   UiLog(ServerHandler* server);
   UiLog(const std::string& server);
   ~UiLog();

   std::ostringstream& info();
   std::ostringstream& err();
   std::ostringstream& warn();
   std::ostringstream& dbg();

protected:
   void output(const std::string& msg);
   void appendType(std::string& s,Type t) const;

   Type type_;
   std::ostringstream os_;
   std::string server_;

private:
   UiLog(const UiLog&);
   UiLog& operator =(const UiLog&);
};

//Overload ostringstream for qt objects
std::ostringstream&  operator <<(std::ostringstream&,const QString &str);
//std::ostringstream&  operator <<(std::ostringstream&,const QModelIndex &);

#endif // UILOG_HPP

