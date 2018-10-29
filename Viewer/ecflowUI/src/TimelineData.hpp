//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//=============================================================

#ifndef TIMELINEDATA_HPP
#define TIMELINEDATA_HPP

#include <string>
#include <vector>
#include <QDateTime>

class TimelineItem
{
public:
    enum Type {UndeterminedType,ServerType,SuiteType,FamilyType,TaskType};

    TimelineItem(const std::string& path,unsigned char status,unsigned int time,bool taskType);
    size_t size() const {return status_.size();}
    //const std::string& name() const {return name_;}
    const std::string& path() const {return path_;}
    bool isTask() const {return type_ ==  TaskType;}
    void add(unsigned char status,unsigned int time);
    void setTypeToTask() {type_ = TaskType;}
    static QDateTime toQDateTime(unsigned int t)
          {return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(t)*1000);}

//protected:
    std::string path_;
    Type type_;
    std::vector<unsigned int> start_;
    std::vector<unsigned int> end_;
    std::vector<unsigned char> status_;
};


class TimelineData
{
public:
    TimelineData() : startTime_(0), endTime_(0) {}
    void loadLogFile(const std::string& logFile,int numOfRows);
    size_t size() const {return  items_.size();}
    const std::vector<TimelineItem>& items() const {return items_;}
    unsigned int startTime() const {return startTime_;}
    unsigned int endTime() const {return endTime_;}
    QDateTime qStartTime() const {return TimelineItem::toQDateTime(startTime_);}
    QDateTime qEndTime() const {return TimelineItem::toQDateTime(endTime_);}
    void clear();

protected:
    int indexOfItem(const std::string&);

    std::vector<TimelineItem> items_;
    int numOfRows_;
    unsigned int startTime_;
    unsigned int endTime_;
};


#endif // TIMELINEDATA_HPP
