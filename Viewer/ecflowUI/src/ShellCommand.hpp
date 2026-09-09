/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ShellCommand_HPP
#define ecflow_viewer_ShellCommand_HPP

#include <sstream>
#include <string>
#include <vector>

#include <QDateTime>
#include <QObject>
#include <QProcess>

#include "CommandOutput.hpp"

class ShellCommand : public QObject {
    Q_OBJECT
public:
    static ShellCommand* run(const std::string&, const std::string&, bool addToDialog = true);

    QString command() const;
    QString commandDef() const { return commandDef_; }
    QDateTime startTime() const { return startTime_; }

protected Q_SLOTS:
    void procFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotStdOutput();
    void slotStdError();

protected:
    ShellCommand(const std::string&, const std::string&, bool);

    QProcess* proc_;
    QString command_;
    QString commandDef_;
    QDateTime startTime_;
    CommandOutput_ptr item_;
    bool addToDialog_;
    static bool envChecked_;
    static bool envHasToBeSet_;
};

#endif /* ecflow_viewer_ShellCommand_HPP */
