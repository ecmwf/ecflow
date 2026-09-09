/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TimelineFileList_HPP
#define ecflow_viewer_TimelineFileList_HPP

#include <QList>
#include <QString>

#include "VFile.hpp"

class TimelineFileListItem {
public:
    TimelineFileListItem(QString fileName,
                         VFile_ptr uncompressedFile,
                         unsigned int startTime,
                         unsigned int endTime,
                         qint64 size)
        : loadable_(true),
          fileName_(fileName),
          startTime_(startTime),
          endTime_(endTime),
          size_(size),
          uncompressedFile_(uncompressedFile) {}

    TimelineFileListItem(QString fileName, qint64 size, QString message = QString())
        : loadable_(false),
          fileName_(fileName),
          startTime_(0),
          endTime_(0),
          size_(size),
          message_(message) {}

    QString dataPath() const {
        return (!uncompressedFile_) ? (fileName_) : (QString::fromStdString(uncompressedFile_->path()));
    }

    bool loadable_;
    QString fileName_;
    unsigned int startTime_;
    unsigned int endTime_;
    qint64 size_;
    QString message_;

private:
    VFile_ptr uncompressedFile_;
};

class TimelineFileList {
public:
    TimelineFileList() = default;
    explicit TimelineFileList(QStringList exprLst);
    TimelineFileList(const TimelineFileList& o)            = default;
    TimelineFileList& operator=(const TimelineFileList& o) = default;

    QList<TimelineFileListItem> items() const { return items_; }
    void clear() { items_.clear(); }
    int loadableCount() const;
    QString firstLoadablePath() const;
    qint64 totalSize() const;

protected:
    void add(QString logFile);

    QList<TimelineFileListItem> items_;
};
#endif /* ecflow_viewer_TimelineFileList_HPP */
