/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_OutputCache_HPP
#define ecflow_viewer_OutputCache_HPP

#include <sstream>

#include <QElapsedTimer>
#include <QMap>
#include <QSet>
#include <QTimer>

#include "OutputClient.hpp"
#include "VFile.hpp"
#include "VInfo.hpp"

class OutputCache;

struct OutputCacheItem
{
    friend class OutputCache;

public:
    OutputCacheItem(QString id, VFile_ptr file);
    VFile_ptr file() const { return file_; }
    bool isAttached() const;
    friend std::ostream& operator<<(std::ostream& stream, const OutputCacheItem& item);

protected:
    void attach();
    void detach();

    QString id_;
    VFile_ptr file_;
    bool used_;
    QElapsedTimer inTimeOut_;
};

class OutputCache : public QObject {
    Q_OBJECT

public:
    explicit OutputCache(QObject* parent = nullptr);
    ~OutputCache() override;

    OutputCacheItem* add(VInfo_ptr info, const std::string& sourcePath, VFile_ptr file);
    OutputCacheItem* attachOne(VInfo_ptr info, const std::string& fileName);
    void detach();
    void clear();
    void print();

protected Q_SLOTS:
    void slotTimeOut();

private:
    explicit OutputCache(const OutputClient&);
    OutputCache& operator=(const OutputCache&);
    void adjustTimer();
    void startTimer();
    void stopTimer();

    QMap<QString, OutputCacheItem*> items_;
    int timeOut_;
    int maxAttachedPeriod_;
    QTimer* timer_;
};

#endif /* ecflow_viewer_OutputCache_HPP */
