/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TriggerScanner_HPP
#define ecflow_viewer_TriggerScanner_HPP

#include <QObject>

class VNode;
class VServer;

class TriggeredScanner : public QObject {
    Q_OBJECT

public:
    explicit TriggeredScanner(QObject* parent)
        : QObject(parent),
          total_(0),
          current_(0),
          batchSize_(100) {}

    void clear();
    void start(VServer*);

Q_SIGNALS:
    void scanStarted();
    void scanFinished();
    void scanProgressed(int percent);

private:
    void scan(VNode*);
    void updateProgress();
    int progress() const;

    int total_;
    int current_;
    int batchSize_;
};

#endif /* ecflow_viewer_TriggerScanner_HPP */
