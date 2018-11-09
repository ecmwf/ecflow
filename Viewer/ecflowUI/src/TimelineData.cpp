//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//=============================================================

#include "TimelineData.hpp"

#include "File_r.hpp"
#include "File.hpp"
#include "NodePath.hpp"
#include "Str.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QStringList>

#include <algorithm>

#include "TimelineData.hpp"
#include "VNState.hpp"

TimelineItem::TimelineItem(const std::string& path,size_t pathHash,unsigned char status,unsigned int time,Type type) :
    path_(path),
    pathHash_(pathHash),
    type_(type)
{
    add(status,time);
}

void TimelineItem::add(unsigned char status,unsigned int time)
{
    if(end_.size() > 0)
        end_[end_.size()-1]=time;

    status_.push_back(status);
    start_.push_back(time);
    end_.push_back(0);
}

void TimelineData::clear()
{
    numOfRows_=0;
    startTime_=0;
    endTime_=0;
    maxReadSize_=0;
    fullRead_=false;
    loadTried_=false;
    loadFailed_=false;
    items_=std::vector<TimelineItem>();
    loadedAt_=QDateTime();
}

void TimelineData::setItemType(int index,TimelineItem::Type type)
{
    items_[index].type_=type;
}

void TimelineData::loadLogFile(const std::string& logFile,size_t maxReadSize,const std::vector<std::string>& suites)
{
    //Clear all collected data
    clear();

    maxReadSize_=maxReadSize;
    fullRead_=false;
    loadTried_=true;
    loadFailed_=false;
    loadedAt_=QDateTime::currentDateTime();

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if( !log_file.ok() )
    {
        loadFailed_=true;
        UiLog().warn() << "TimelineData::loadLogFile: Could not open log file " << logFile ;
        throw std::runtime_error("Could not open log file: " + logFile);
    }

    fullRead_=true;
    size_t fSize=0;
    if(maxReadSize_ > 0)
    {
        QFileInfo fInfo(QString::fromStdString(logFile));
        fSize=fInfo.size();

        if(fSize > maxReadSize_)
        {
            fullRead_=false;
            log_file.setPos(fSize - maxReadSize_);
        }
    }

    std::string line;

    while ( log_file.good() )
    {
        log_file.getline(line); // default delimiter is /n

        // The log file format we are interested is :
        // 0             1         2            3
        // MSG:[HH:MM:SS D.M.YYYY] chd:fullname [path +additional information]
        // MSG:[HH:MM:SS D.M.YYYY] --begin      [args | path(optional) ]    :<user>

        //LOG:[22:45:30 21.4.2018]  complete: path
        //LOG:[22:45:30 21.4.2018]  submitted: path job_size:16408

        /// We are only interested in status changes (i.e LOG:)
        if (line.empty())
            continue;

        if (line[0] != 'L')
            continue;

        std::string::size_type log_pos = line.find("LOG:");
        if (log_pos != 0)
            continue;

        /// LOG:[HH:MM:SS D.M.YYYY] status: fullname [+additional information]
        /// EXTRACT the date
        std::string::size_type first_open_bracket = line.find('[');
        if ( first_open_bracket == std::string::npos)
        {
            assert(false);
            continue;
        }
        //line.erase(0,first_open_bracket+1);

        std::string::size_type first_closed_bracket = line.find(']',first_open_bracket);
        if ( first_closed_bracket ==  std::string::npos)
        {
            //assert(false);
            continue;
        }
        std::string time_stamp = line.substr(first_open_bracket+1,first_closed_bracket-first_open_bracket-1);

        //ecf::Str::split(time_stamp, new_time_stamp);
        //if (new_time_stamp.size() != 2)
        //    continue;

        //line.erase(0,first_closed_bracket+1);

        ///extract the status
        std::string::size_type first_colon = line.find(':',first_closed_bracket);
        if(first_colon == std::string::npos)
            continue;

        std::string::size_type first_char = line.find_first_not_of(' ',first_closed_bracket+1);
        if(first_char  == std::string::npos)
            continue;

        std::string status=line.substr(first_char,first_colon-first_char);

        //get the status id
        unsigned char statusId;
        if(VNState* vn=VNState::find(status))
            statusId=vn->ucId();
        else
            continue;

        //extract the full name
        first_char =  line.find_first_not_of(' ', first_colon+1);
        if(first_char  == std::string::npos)
              continue;

        std::string::size_type next_ws = line.find(' ', first_char+1);
        std::string name;
        if(next_ws  == std::string::npos)
        {
            name=line.substr(first_char);
        }
        else
        {
            name=line.substr(first_char,next_ws-first_char);
        }

        //Filter by suites
        if(!suites.empty() && name.size() > 1 && name[0] == '/')
        {
            std::string suite;
            std::string::size_type next_sep=name.find("/",1);
            if(next_sep != std::string::npos)
            {
                suite=name.substr(1,next_sep-1);
            }
            else
            {
                suite=name.substr(1);
            }

            if(std::find(suites.begin(),suites.end(),suite) == suites.end())
                continue;
        }

        //Convert status time into
        unsigned int statusTime=QDateTime::fromString(QString::fromStdString(time_stamp),
                       "hh:mm:ss d.M.yyyy").toMSecsSinceEpoch()/1000;

        if(startTime_ == 0)
            startTime_=statusTime;

        if(statusTime > endTime_)
            endTime_=statusTime;

        int idx=indexOfItem(name);       
        if(idx != -1)
        {
            items_[idx].add(statusId,statusTime);
            if(items_[idx].type_ == TimelineItem::UndeterminedType)
            {
                items_[idx].type_=guessNodeType(line,status,next_ws);
            }
        }
        else
        {                               
            items_.push_back(TimelineItem(name,pathHash_(name),statusId,statusTime,
                                          guessNodeType(line,name,status,next_ws)));
        }

        numOfRows_++;
    }

    //guessNodeType();
}

void TimelineData::guessNodeType()
{
    std::vector<size_t> taskIndex;
    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i].isTask())
            taskIndex.push_back(i);
    }

    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i].type_ == TimelineItem::UndeterminedType)
        {
            for(size_t j=0; j < taskIndex.size(); j++)
            {
                if(items_[j].path_.find(items_[i].path_) != std::string::npos)
                {
                    items_[i].type_=TimelineItem::FamilyType;
                    break;
                }
            }
        }
    }
}

TimelineItem::Type TimelineData::guessNodeType(const std::string& line,const std::string& name,
                                  const std::string& status,
                                  std::string::size_type next_ws) const
{
    if(name.find_last_of("/") == 0)
    {
        return TimelineItem::SuiteType;
    }

    return guessNodeType(line,status,next_ws);
}

TimelineItem::Type TimelineData::guessNodeType(const std::string& line,
                                  const std::string& status,
                                  std::string::size_type next_ws) const
{
    //Try to figure out if it is a taks when status=submitted. If there is
    //an item with "job_size:" it must be a task.
    if(status == "submitted" && next_ws  != std::string::npos)
    {
       if(line.find("job_size:",next_ws) != std::string::npos)
           return TimelineItem::TaskType;
    }
    return TimelineItem::UndeterminedType;
}

int TimelineData::indexOfItem(const std::string& p)
{
    size_t hval=pathHash_(p);
    bool redo=false;
    for(size_t i=0; i < items_.size(); i++)
    {
        if(items_[i].pathHash_ == hval)
        {
            if(items_[i].path_ == p)
                return i;
            else
            {
                redo=true;
                break;
            }
        }
    }

    //If the hash is not unique!
    if(redo)
    {
        for(size_t i=0; i < items_.size(); i++)
        {
            if(items_[i].path_ == p)
                    return i;
        }
    }

    return -1;
}
