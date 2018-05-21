//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef LOGDATA_HPP
#define LOGDATA_HPP

#include <QDateTime>

#include <string>
#include <vector>

class LogDataItem
{
    friend class LogData;

public:
    explicit LogDataItem(const std::string&,qint64 refTimeInMs);
    enum Type {NoType,MessageType,ErrorType,LogType,WarningType,DebugType};
    static qint64 getTimeInMs(const std::string& line);

protected:
    Type type_;
    unsigned int time_; //in seconds since the refTime
    std::string entry_;
};


class LogData
{
    friend class LogDataItem;

public:
    LogData() : refTimeInMs_(0) {}

    std::size_t size() const {return data_.size();}
    bool isEmpty() const {return size() == 0;}
    void clear() { refTimeInMs_=0; fileName_.clear(); data_.clear();}

    QDateTime date(int idx) const {return
                QDateTime::fromMSecsSinceEpoch(refTimeInMs_ +
                           data_[idx].time_*1000); }

    QString  entry(int idx) const {return QString::fromStdString(data_[idx].entry_);}
    LogDataItem::Type type(int idx) const {return data_[idx].type_;}
    bool indexOfPeriod(qint64 start,qint64 end,size_t& idxStart,size_t& idxEnd,qint64 tolerance);


    void loadFromFile(const std::string& logFile);
    void loadFromText(const std::string& txt);
    void loadFromText(const std::vector<std::string>& txtVec);
    void appendFromText(const std::string& txt);
    void appendFromText(const std::vector<std::string>& txtVec);

protected:
    qint64 refTimeInMs_;
    std::string fileName_;
    std::vector<LogDataItem> data_;
};


#endif // LOGDATA_HPP
