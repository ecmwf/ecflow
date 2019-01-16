//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef COMMANDOUTPUT_HPP
#define COMMANDOUTPUT_HPP

#include <QColor>
#include <QDateTime>
#include <QString>
#include <QObject>
#include <QVector>

#include <memory>

class CommandOutput;
typedef std::shared_ptr<CommandOutput> CommandOutput_ptr;

class CommandOutputHandler;

class CommandOutput
{
    friend class CommandOutputHandler;

public:
    enum Status {RunningStatus,FinishedStatus,FailedStatus};

    QString command() const {return command_;}
    QString commandDefinition() const {return commandDef_;}
    QDateTime runTime() const {return runTime_;}
    QString output() const {return output_;}
    QString error() const {return error_;}
    Status status() const {return status_;}
    QString statusStr() const;
    QColor statusColour() const;
    bool isEnabled() const {return enabled_;}

protected:
    CommandOutput(QString cmd,QString cmdDef,QDateTime runTime);

    void appendOutput(QString,int,bool&);
    void appendError(QString,int,bool&);
    void setStatus(Status s) {status_=s;}
    void setEnabled(bool b) {enabled_=b;}

    bool enabled_;
    QString command_;
    QString commandDef_;
    QDateTime runTime_;
    QString output_;
    QString error_;
    Status status_;
};

class CommandOutputHandler : public QObject
{
    Q_OBJECT
public:
    static CommandOutputHandler* instance();

    void appendOutput(CommandOutput_ptr,QString);
    void appendError(CommandOutput_ptr,QString);
    void finished(CommandOutput_ptr);
    void failed(CommandOutput_ptr);

    CommandOutput_ptr addItem(QString cmd,QString cmdDef,QDateTime runTime);
    QVector<CommandOutput_ptr> items() const {return items_;}
    int itemCount() const {return items_.count();}
    int indexOfItem(CommandOutput_ptr) const;

Q_SIGNALS:
    void itemAddBegin();
    void itemAddEnd();
    void itemOutputAppend(CommandOutput_ptr,QString);
    void itemErrorAppend(CommandOutput_ptr,QString);
    void itemOutputReload(CommandOutput_ptr);
    void itemErrorReload(CommandOutput_ptr);
    void itemStatusChanged(CommandOutput_ptr);
    void itemsReloaded();

protected:
    CommandOutputHandler(QObject*);
    void checkItems();

    static CommandOutputHandler* instance_;
    int maxNum_;
    int maxOutputSize_;
    int maxErrorSize_;
    QVector<CommandOutput_ptr> items_;
};


#endif // COMMANDOUTPUT_HPP
