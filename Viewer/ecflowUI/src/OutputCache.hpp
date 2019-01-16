//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef OUTPUTCHACHE_HPP_
#define OUTPUTCHACHE_HPP_

#include <QMap>
#include <QTime>
#include <QTimer>
#include <QSet>

#include <sstream>

#include "OutputClient.hpp"
#include "VFile.hpp"
#include "VInfo.hpp"


class OutputCache;

struct OutputCacheItem
{
  friend class OutputCache;

public:
    OutputCacheItem(QString id,VFile_ptr file);
    VFile_ptr file() const {return file_;}
    bool isAttached() const;
    friend std::ostream& operator<<(std::ostream& stream,const OutputCacheItem& item);

protected:
    void attach();
    void detach();

    QString id_;
    VFile_ptr file_;   
    bool used_;
    QTime inTimeOut_;
};

class OutputCache:  public QObject
{
    Q_OBJECT

public:   
    OutputCache(QObject* parent=nullptr);
    ~OutputCache() override;

    OutputCacheItem* add(VInfo_ptr info,const std::string& sourcePath,VFile_ptr file);
    OutputCacheItem* attachOne(VInfo_ptr info,const std::string& fileName);
    void detach();
    void clear();
    void print();

protected Q_SLOTS:
    void slotTimeOut();

private:
    OutputCache(const OutputClient&);
    OutputCache& operator=(const OutputCache&);
    void adjustTimer();
    void startTimer();
    void stopTimer();

    QMap<QString,OutputCacheItem*> items_;
    int timeOut_;
    int maxAttachedPeriod_;
    QTimer* timer_;
};

#endif // OUTPUTCHACHE_HPP

