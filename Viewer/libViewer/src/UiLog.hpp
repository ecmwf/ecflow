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
#include <vector>
#include <boost/current_function.hpp>

class QString;
class QModelIndex;
class QStringList;
class QVariant;
class QPoint;
class QRegion;
class QRect;
class QDateTime;

#define UI_FUNCTION_LOG UiFunctionLog __fclog(BOOST_CURRENT_FUNCTION);
//#define UI_FUNCTION_LOG_S(server) UiFunctionLog __fclog(server,BOOST_CURRENT_FUNCTION);

class UiFunctionLog
{
public:
    UiFunctionLog(const std::string& server,const std::string& funcName);
    UiFunctionLog(const std::string& funcName);
    ~UiFunctionLog();

    std::string logEnter() const;
    std::string logLeave() const;
protected:
    void init();

    std::string serverName_;
    std::string funcName_;
};

class UiLog
{
public:
   enum Type {INFO,WARN,ERROR,DBG};

   UiLog() : type_(INFO) {}   
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

//Overload ostringstream for various objects
std::ostream&  operator <<(std::ostream&,const std::vector<std::string>&);

//Overload ostringstream for qt objects
std::ostream&  operator <<(std::ostream&,const QString &);
std::ostream&  operator <<(std::ostream&,const QModelIndex&);
std::ostream&  operator <<(std::ostream&,const QVariant&);
std::ostream&  operator <<(std::ostream&,const QStringList&);
std::ostream&  operator <<(std::ostream&,const QRegion&);
std::ostream&  operator <<(std::ostream&,const QRect&);
std::ostream&  operator <<(std::ostream&,const QPoint&);
std::ostream&  operator <<(std::ostream&,const QDateTime&);

#endif // UILOG_HPP

