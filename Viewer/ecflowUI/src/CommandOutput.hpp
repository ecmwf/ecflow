/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_CommandOutput_HPP
#define ecflow_viewer_CommandOutput_HPP

#include <memory>

#include <QColor>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

#include "VProperty.hpp"

class CommandOutput;
using CommandOutput_ptr = std::shared_ptr<CommandOutput>;

class CommandOutputHandler;

class CommandOutput {
    friend class CommandOutputHandler;

public:
    enum Status { RunningStatus, FinishedStatus, FailedStatus };

    QString command() const { return command_; }
    QString commandDefinition() const { return commandDef_; }
    QDateTime runTime() const { return runTime_; }
    QString output() const { return output_; }
    QString error() const { return error_; }
    Status status() const { return status_; }
    QString statusStr() const;
    QColor statusColour() const;
    bool isEnabled() const { return enabled_; }

protected:
    CommandOutput(QString cmd, QString cmdDef, QDateTime runTime);

    void appendOutput(QString, int, bool&);
    void appendError(QString, int, bool&);
    void setStatus(Status s) { status_ = s; }
    void setEnabled(bool b) { enabled_ = b; }

    bool enabled_;
    QString command_;
    QString commandDef_;
    QDateTime runTime_;
    QString output_;
    QString error_;
    Status status_;
};

class CommandOutputHandler : public QObject {
    Q_OBJECT
public:
    enum CreateContext { NormalContext, StdOutContext, StdErrContext };
    static CommandOutputHandler* instance();

    void appendOutput(CommandOutput_ptr, QString);
    void appendError(CommandOutput_ptr, QString);
    void finished(CommandOutput_ptr);
    void failed(CommandOutput_ptr);

    CommandOutput_ptr addItem(QString cmd, QString cmdDef, QDateTime runTime, CreateContext context);
    QVector<CommandOutput_ptr> items() const { return items_; }
    int itemCount() const { return items_.count(); }
    int indexOfItem(CommandOutput_ptr) const;

Q_SIGNALS:
    void itemAddBegin();
    void itemAddEnd();
    void itemOutputAppend(CommandOutput_ptr, QString);
    void itemErrorAppend(CommandOutput_ptr, QString);
    void itemOutputReload(CommandOutput_ptr);
    void itemErrorReload(CommandOutput_ptr);
    void itemStatusChanged(CommandOutput_ptr);
    void itemsReloaded();

protected:
    CommandOutputHandler(QObject* parent);
    void checkItems();
    bool needToShowStdOut();
    bool needToShowStdErr();

    static CommandOutputHandler* instance_;
    int maxNum_{25};
    int maxOutputSize_{1000000};
    int maxErrorSize_{30000};
    QVector<CommandOutput_ptr> items_;
    VProperty* showDialogStdOutProp_{nullptr};
    VProperty* showDialogStdErrProp_{nullptr};
};

#endif /* ecflow_viewer_CommandOutput_HPP */
