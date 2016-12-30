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

class ServerHandler;

#if 0
class UserMessage
{
//Q_OBJECT
public:
    UserMessage();

    enum MessageType {INFO, WARN, ERROR, DBG};  // note: cannot have DEBUG because of possible -DDEBUG in cpp!

    static void setEchoToCout(bool toggle) {echoToCout_ = toggle;}
    static void message(MessageType type, bool popup, const std::string& message);
    static bool confirm(const std::string& message);

    static void debug(const std::string& message);
    static void qdebug(QString message);
    static std::string toString(int);

private:
    static bool echoToCout_;


};
#endif

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

#if 0
   std::ostringstream& info(const std::string& server);
   std::ostringstream& err(const std::string& server);
   std::ostringstream& warn(const std::string& server);
   std::ostringstream& dbg(const std::string& server);
#endif

protected:
   void output(const std::string& msg);
   void appendType(std::string& s,Type t) const;
   void timeStamp(std::string& time_stamp);

   Type type_;
   std::ostringstream os_;
   std::string server_;

private:
   UiLog(const UiLog&);
   UiLog& operator =(const UiLog&);
};

#endif // UILOG_HPP

