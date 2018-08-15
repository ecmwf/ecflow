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

class TimelineItem
{
public:
    TimelineItem(const std::string& path,unsigned char status,unsigned int time);
    size_t size() const {return status_.size();}
    //const std::string& name() const {return name_;}
    const std::string& path() const {return path_;}
    void add(unsigned char status,unsigned int time);

protected:
    std::string path_;
    std::vector<unsigned int> start_;
    std::vector<unsigned int> end_;
    std::vector<unsigned char> status_;
};


class TimelineData
{
public:
    TimelineData() {}
    void loadLogFile(const std::string& logFile,int numOfRows);
    size_t size() const {return  items_.size();}
    const std::vector<TimelineItem>& items() const {return items_;}

protected:

    void clear(){}
    int indexOfItem(const std::string&);

    std::vector<TimelineItem> items_;
    int numOfRows_;

};


#endif // TIMELINEDATA_HPP
