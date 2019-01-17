//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGEREDSCANNER_HPP
#define TRIGGEREDSCANNER_HPP

#include <QObject>

class VNode;
class VServer;

class TriggeredScanner : public QObject
{
Q_OBJECT

public:
    TriggeredScanner(QObject* parent) : QObject(parent), total_(0), current_(0), batchSize_(100) {}

    void clear();
    void start(VServer*);

Q_SIGNALS:
    void scanStarted();
    void scanFinished();
    void scanProgressed(int percent);

private:
    void scan(VNode*);
    void updateProgress();
    int  progress() const;

    int total_;
    int current_;
    int batchSize_;
};

#endif // TRIGGEREDSCANNER_HPP

