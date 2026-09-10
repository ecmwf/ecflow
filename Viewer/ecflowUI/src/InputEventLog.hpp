/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_InputEventLog_HPP
#define ecflow_viewer_InputEventLog_HPP

#include <QObject>
#include <QTextStream>

class QCloseEvent;
class QContextMenuEvent;
class QFile;
class QMouseEvent;
class LogTruncator;

class InputEventLog : public QObject {
    Q_OBJECT
public:
    ~InputEventLog() override;

    void start();
    void stop();

    static InputEventLog* instance();

protected Q_SLOTS:
    void truncateLogBegin();
    void truncateLogEnd();

protected:
    explicit InputEventLog(QObject* parent = nullptr);

    bool eventFilter(QObject* obj, QEvent* event) override;
    void logMousePress(QObject* obj, QMouseEvent* e);
    void logMouseRelease(QObject* obj, QMouseEvent* e);
    void logClose(QObject* obj, QCloseEvent* e);
    void logContextMenu(QObject* obj, QContextMenuEvent* e);

    static InputEventLog* instance_;
    bool paused_{false};
    QFile* outFile_;
    QTextStream out_;
    LogTruncator* truncator_;
};

#endif /* ecflow_viewer_InputEventLog_HPP */
