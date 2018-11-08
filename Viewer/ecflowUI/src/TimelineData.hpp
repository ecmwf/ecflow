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
#include <functional>

#include <QDateTime>

class TimelineItem
{
public:
    enum Type {UndeterminedType,ServerType,SuiteType,FamilyType,TaskType};

    TimelineItem() : pathHash_(0), type_(UndeterminedType) {}
    TimelineItem(const std::string& path,size_t pathHash,unsigned char status,unsigned int time,Type type=UndeterminedType);
    size_t size() const {return status_.size();}
    const std::string& path() const {return path_;}
    Type type() const {return type_;}
    void setType(Type t) {type_ = t;}
    bool isTask() const {return type_ ==  TaskType;}
    void add(unsigned char status,unsigned int time);

    static QDateTime toQDateTime(unsigned int t)
          {return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(t)*1000,Qt::UTC);}

//protected:
    std::string path_;
    size_t pathHash_;
    Type type_;
    std::vector<unsigned int> start_;
    std::vector<unsigned int> end_;
    std::vector<unsigned char> status_;
};


class TimelineData
{
public:
    TimelineData() : startTime_(0), endTime_(0), maxReadSize_(0), fullRead_(false) {}
    void loadLogFile(const std::string& logFile,size_t maxReadSize);
    size_t size() const {return  items_.size();}
    const std::vector<TimelineItem>& items() const {return items_;}
    unsigned int startTime() const {return startTime_;}
    unsigned int endTime() const {return endTime_;}
    QDateTime qStartTime() const {return TimelineItem::toQDateTime(startTime_);}
    QDateTime qEndTime() const {return TimelineItem::toQDateTime(endTime_);}
    void clear();    
    void setItemType(int index,TimelineItem::Type type);
    bool isFullRead() const {return fullRead_;}

protected:
    int indexOfItem(const std::string&);
    void guessNodeType();
    TimelineItem::Type guessNodeType(const std::string& line,const std::string& name,
                                      const std::string& status,
                                      std::string::size_type next_ws) const;
    TimelineItem::Type guessNodeType(const std::string& line,
                                      const std::string& status,
                                      std::string::size_type next_ws) const;

    std::vector<TimelineItem> items_;
    int numOfRows_;
    unsigned int startTime_;
    unsigned int endTime_;
    std::hash<std::string> pathHash_;
    size_t maxReadSize_;
    bool fullRead_;
};


#endif // TIMELINEDATA_HPP
