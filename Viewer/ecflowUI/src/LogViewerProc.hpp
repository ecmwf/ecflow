//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef LOGVIEWERPROC_HPP
#define LOGVIEWERPROC_HPP

#include <string>
#include <vector>
#include <sstream>

#include <QDateTime>
#include <QObject>

#include <QProcess>

class ServerHandler;
class QLocalSocket;

class LogViewerProc : public QObject
{
    Q_OBJECT
public:
    LogViewerProc();

    //static ShellCommand* run(const std::string&,const std::string&);

    void addToWin(ServerHandler*);
    //QString command() const;
    //QString commandDef() const {return commandDef_;}
    //QDateTime startTime() const {return startTime_;}

protected Q_SLOTS:
    //void procFinished(int exitCode, QProcess::ExitStatus exitStatus);
    //void slotStdOutput();
    //void slotStdError();

protected:
    //LogViewerProc(const std::string&,const std::string&);
    void start(QStringList);

    QProcess *proc_;
    QString program_;
    QString logViewerId_;
    QLocalSocket* socket_;
};

#endif // LOGVIEWERPROC_HPP
