//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//=============================================================================

#ifndef TIMELINEFILELIST_HPP
#define TIMELINEFILELIST_HPP

#include <QString>
#include <QList>

class TimelineFileListItem
{
public:
    TimelineFileListItem(QString fileName,
                         unsigned int startTime,unsigned int endTime, qint64 size) :
        loadable_(true),fileName_(fileName), startTime_(startTime), endTime_(endTime), size_(size) {}

    TimelineFileListItem(QString fileName, qint64 size, QString message=QString()) :
        loadable_(false),fileName_(fileName), startTime_(0), endTime_(0), size_(size),
        message_(message) {}

    bool loadable_;
    QString fileName_;
    unsigned int startTime_;
    unsigned int endTime_;
    qint64 size_;
    QString message_;
};

class TimelineFileList
{
public:
    TimelineFileList() {}
    TimelineFileList(QStringList exprLst);
    TimelineFileList(const TimelineFileList& o) {items_=o.items();}
    QList<TimelineFileListItem> items() const {return items_;}
    void clear() {items_.clear();}

protected:
    void add(QString logFile);

    QList<TimelineFileListItem> items_;
};
#endif // TIMELINEFILELIST_HPP
