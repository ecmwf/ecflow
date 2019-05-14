//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//=============================================================================

#include "TimelineFileList.hpp"

#include <vector>
#include <algorithm>

#include <QFileInfo>

#include "File_r.hpp"
#include "TimelineData.hpp"

//=================================================================
//
// TimelineFileList
//
//=================================================================

TimelineFileList::TimelineFileList(QStringList exprLst)
{
    Q_FOREACH(QString s,exprLst)
    {
        add(s);
    }

    //sort items by startTime
    std::vector<std::pair<size_t, unsigned int> > sortVec;
    for(int i = 0; i < items_.size(); i++)
    {
        sortVec.push_back(std::make_pair(i,items_[i].startTime_));
    }

    std::sort(sortVec.begin(), sortVec.end());

    QList<TimelineFileListItem> itemsTmp=items_;
    items_.clear();

    for(int i = 0; i < itemsTmp.size(); i++)
    {
        items_ << itemsTmp[sortVec[i].first];
    }
}

void TimelineFileList::add(QString logFile)
{
    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile.toStdString());
    if(!log_file.ok() )
    {
        //UiLog().warn() << "TimelineData::loadLogFile: Could not open log file " << logFile ;
        //throw std::runtime_error("Could not open log file: " + logFile);
        items_ << TimelineFileListItem(logFile,"Could not open log file");
        return;
    }

    std::string line;

    unsigned int startTime=0;
    unsigned int endTime=0;

    //get the first time
    while(log_file.good())
    {
        log_file.getline(line); // default delimiter is /n

        std::string name;
        unsigned char statusId=0;
        unsigned int statusTime=0;
        if(TimelineData::parseLine(line,name,statusId,statusTime))
        {
           startTime=statusTime;
           break;
        }
    }

    if(startTime == 0)
    {
        items_ << TimelineFileListItem(logFile,"No status change found");
        return;
    }

    //get the last time
    QFileInfo fInfo(logFile);
    size_t fSize=fInfo.size();

    log_file.setPos(0);
    if(fSize > 10000)
    {
        log_file.setPos(fSize - 10000);
    }

    //get the last time
    while(log_file.good())
    {
        log_file.getline(line); // default delimiter is /n

        std::string name;
        unsigned char statusId=0;
        unsigned int statusTime=0;
        if(TimelineData::parseLine(line,name,statusId,statusTime))
        {
            endTime=statusTime;
        }
    }

    items_ << TimelineFileListItem(logFile,startTime,endTime);
}
