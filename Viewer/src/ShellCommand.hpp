//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SHELLCOMMAND_HPP
#define SHELLCOMMAND_HPP

#include <string>
#include <vector>
#include <sstream>

#include <QDateTime>
#include <QObject>

#include <QProcess>

class ShellCommand : public QObject
{
    Q_OBJECT
public:
    static ShellCommand* run(const std::string&,const std::string&);

    QString command() const;
    QString commandDef() const {return commandDef_;}
    QDateTime startTime() const {return startTime_;}

protected Q_SLOTS:
    void procFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotStdOutput();
    void slotStdError();

protected:
    ShellCommand(const std::string&,const std::string&);

    QProcess *proc_;
    QString command_;
    QString commandDef_;
    QDateTime startTime_;
    static bool envChecked_;
    static bool envHasToBeSet_;
};

#endif // SHELLCOMMAND_HPP
