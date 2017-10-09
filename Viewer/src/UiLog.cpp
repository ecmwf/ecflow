//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "UiLog.hpp"

#include <assert.h>
#include <iostream>

#include <QDebug>
#include <QModelIndex>
#include <QRegion>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "DirectoryHandler.hpp"
#include "LogTruncator.hpp"
#include "ServerHandler.hpp"
#include "TimeStamp.hpp"

static LogTruncator *truncator=0;

//---------------------------------
// UiFunctionLog
//---------------------------------

UiFunctionLog::UiFunctionLog(ServerHandler* server,const std::string& funcName) :
    funcName_(funcName)
{
    if(server)
        serverName_=server->longName();

    init();
    UiLog(serverName_).dbg() << logEnter();
}

UiFunctionLog::UiFunctionLog(const std::string& funcName) :
    funcName_(funcName)
{
    init();
    UiLog(serverName_).dbg() << logEnter();
}

UiFunctionLog::~UiFunctionLog()
{
    UiLog(serverName_).dbg() << logLeave();
}

void UiFunctionLog::init()
{
    std::size_t pos;
    if((pos=funcName_.find_first_of("(")) != std::string::npos)
    {
        std::size_t pos1=funcName_.rfind(" ",pos);
        if(pos1 != std::string::npos && pos1+1  < pos)
            funcName_=funcName_.substr(pos1+1,pos-pos1-1);
    }
}

std::string UiFunctionLog::logEnter() const
{
    return funcName_ + " -->";
}

std::string UiFunctionLog::logLeave() const
{
    return "<-- " + funcName_;
}

//---------------------------------
// UiLog
//---------------------------------

UiLog::UiLog(ServerHandler* sh) :
    type_(INFO), server_(sh->longName())
{}

UiLog::UiLog(const std::string& server) :
    type_(INFO), server_(server)
{}


UiLog::~UiLog()
{
    output(os_.str());
}

void UiLog::appendType(std::string& s,Type t) const
{
    switch(t)
    {
    case DBG: s.append("DBG:"); break;
    case INFO: s.append("INF:"); break;
    case WARN: s.append("WAR:"); break;
    case ERROR: s.append("ERR:"); break;
    default: assert(false); break;
    }
}

std::ostringstream& UiLog::info()
{
    type_=INFO;
    return os_;
}

std::ostringstream& UiLog::err()
{
    type_=ERROR;
    return os_;
}

std::ostringstream& UiLog::warn()
{
    type_=WARN;
    return os_;
}

std::ostringstream& UiLog::dbg()
{
    type_=DBG;
    return os_;
}

void UiLog::output(const std::string& msg)
{
    std::string ts;
    ecf::TimeStamp::now_in_brief(ts);

    std::string s;
    appendType(s,type_);

    if(server_.empty())
        s+=" " + ts + msg;
    else
        s+=" " + ts + "[" + server_ + "] " + msg;

    std::cout << s << std::endl;

}

void UiLog::enableTruncation()
{
    if(!truncator)
        truncator=new LogTruncator(QString::fromStdString(DirectoryHandler::uiLogFileName()),
                                     86400*1000,10*1024*1024,1000);
}


//------------------------------------------
// Overload ostringstream for qt objects
//------------------------------------------
std::ostream&  operator <<(std::ostream &stream,const QString &str)
{
   stream << str.toStdString();
   return stream;
}

std::ostream& operator <<(std::ostream &stream,const QModelIndex& idx)
{
    QString s;
    QDebug ts(&s);
    ts << idx;
    stream << s.toStdString();
    return stream;
}

std::ostream&  operator <<(std::ostream &stream,const QVariant &v)
{
   QString s;
   QDebug ts(&s);
   ts << v;
   stream << s.toStdString();
   return stream;
}

std::ostream&  operator <<(std::ostream &stream,const QStringList &lst)
{
   QString s;
   QDebug ts(&s);
   ts << lst;
   stream << s.toStdString();
   return stream;
}

std::ostream&  operator <<(std::ostream &stream,const QRegion &r)
{
    QString s;
    QDebug ts(&s);
    ts << r;
    stream << s.toStdString();
    return stream;
}

std::ostream&  operator <<(std::ostream &stream,const QRect &r)
{
    QString s;
    QDebug ts(&s);
    ts << r;
    stream << s.toStdString();
    return stream;
}

std::ostream&  operator <<(std::ostream &stream,const QPoint &p)
{
    QString s;
    QDebug ts(&s);
    ts << p;
    stream << s.toStdString();
    return stream;
}
