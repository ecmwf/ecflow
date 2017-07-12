//============================================================================
// Copyright 2009-2017 ECMWF.
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
class QStringList;
class QVariant;
class QPoint;
class QRegion;
class QRect;

class ServerHandler;

class UiLoggable
{
public:
    UiLoggable(const std::string& className) : className_(className) {}
    std::string className_;
};

class UiFunctionLog
{
public:
    UiFunctionLog(UiLoggable *obj,const std::string& funcName);
    ~UiFunctionLog();
    std::string logEnter() const;
    std::string logLeave() const;

    UiLoggable* obj_;
    std::string funcName_;
};


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

   static void enableTruncation();

   static std::string toString(int i) {
        std::stringstream ss;
        ss << i;
        return ss.str();
   }


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
std::ostream&  operator <<(std::ostream&,const QString &);
std::ostream&  operator <<(std::ostream&,const QModelIndex&);
std::ostream&  operator <<(std::ostream&,const QVariant&);
std::ostream&  operator <<(std::ostream&,const QStringList&);
std::ostream&  operator <<(std::ostream&,const QRegion&);
std::ostream&  operator <<(std::ostream&,const QRect&);
std::ostream&  operator <<(std::ostream&,const QPoint&);

#endif // UILOG_HPP

