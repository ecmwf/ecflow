//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LogData.hpp"

#include "UiLog.hpp"

LogDataItem::LogDataItem(const std::string& line,qint64 refTimeInMs) : type_(NoType)
{
    //Format is as follows:
    //MSG:[06:46:44 23.4.2018] chd:complete .....

    if(line.size() == 0)
        return;

    std::string::size_type pos = line.find("[");
    if(pos == std::string::npos)
        return;

    std::string::size_type pos1 = line.find("]",pos);
    if(pos1 == std::string::npos || pos1 == pos+1)
        return;

    std::string t=line.substr(0,pos);
    std::string d=line.substr(pos+1,pos1-pos-1);

    if(pos1+1 < line.size())
        entry_=line.substr(pos1+1);

    time_=(QDateTime::fromString(QString::fromStdString(d),
                                "hh:mm:ss dd.M.yyyy").toMSecsSinceEpoch()-refTimeInMs)/1000;
    if(t == "MSG:")
    {
        type_=MessageType;
    }
    else if(t == "LOG:")
    {
        type_=LogType;
    }
    else if(t == "ERR:")
    {
        type_=ErrorType;
    }
    else if(t == "WAR:")
    {
        type_=WarningType;
    }
    else if(t == "DBG:")
    {
        type_=DebugType;
    }
}

qint64 LogDataItem::getTimeInMs(const std::string& line)
{
    if(line.size() == 0)
        return 0;

    std::string::size_type pos = line.find("[");
    if(pos == std::string::npos)
        return 0;

    std::string::size_type pos1 = line.find("]",pos);
    if(pos1 == std::string::npos || pos1 == pos+1)
        return 0;

    QString d=QString::fromStdString(line.substr(pos+1,pos1-pos-1));

    return QDateTime::fromString(d,"hh:mm:ss dd.M.yyyy").toMSecsSinceEpoch();
}

void LogData::loadFromText(const std::string& txt)
{
    data_.clear();

    QString in=QString::fromStdString(txt);
    Q_FOREACH(QString s,in.split("\n"))
    {
        if(!s.simplified().isEmpty())
        {
            if(refTimeInMs_==0)
                refTimeInMs_=LogDataItem::getTimeInMs(s.toStdString());

            data_.push_back(LogDataItem(s.toStdString(),refTimeInMs_));
        }
    }
}

void LogData::loadFromText(const std::vector<std::string>& txtVec)
{
    data_.clear();
    appendFromText(txtVec);
}

void LogData::appendFromText(const std::vector<std::string>& txtVec)
{
    for(std::size_t i=0; i < txtVec.size(); i++)
    {
        QString s=QString::fromStdString(txtVec[i]);
        if(!s.simplified().isEmpty())
        {
            if(refTimeInMs_==0)
                refTimeInMs_=LogDataItem::getTimeInMs(s.toStdString());

            data_.push_back(LogDataItem(s.toStdString(),refTimeInMs_));
        }
    }
}
