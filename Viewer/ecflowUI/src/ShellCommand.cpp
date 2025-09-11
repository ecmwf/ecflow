/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ShellCommand.hpp"

#include <cstdio>

#include <QDebug>
#include <QProcess>
#include <QString>
#include <QtGlobal>

#include "CommandOutput.hpp"
#include "CommandOutputDialog.hpp"
#include "DirectoryHandler.hpp"
#include "UiLog.hpp"
#include "VFile.hpp"

// static std::vector<ShellCommand*> commands;

bool ShellCommand::envChecked_    = false;
bool ShellCommand::envHasToBeSet_ = false;

ShellCommand::ShellCommand(const std::string& cmdStr, const std::string& cmdDefStr, bool addToDialog)
    : QObject(nullptr),
      proc_(nullptr),
      commandDef_(QString::fromStdString(cmdDefStr)),
      addToDialog_(addToDialog) {
    QString cmdIn = QString::fromStdString(cmdStr);

    // A valid shell command must start with "sh "
    if (!cmdIn.startsWith("sh ")) {
        deleteLater();
        return;
    }

    command_ = cmdIn.mid(3);

    proc_ = new QProcess(this);

    // Some commands/script are located in bin, while some other is share/ecflow/etc

    // Check environment - only has to be once
    if (!envChecked_) {
        envChecked_      = true;
        QStringList pLst = {QString::fromStdString(DirectoryHandler::etcDir()),
                            QString::fromStdString(DirectoryHandler::exeDir())};
        for (auto exeDir : pLst) {
            // If there is an exe dir we check if it is added to the PATH env
            // variable
            if (!exeDir.isEmpty()) {
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                QString envPath         = env.value("PATH");
                if (!envPath.contains(exeDir)) {
                    envHasToBeSet_ = true;
                    break;
                }
            }
        }
    }

    // If the shell command runs ecflow_client it has to be in the PATH env variable.
    // When we develop the ui it is not the case so we need to add its path to PATH
    // whenever is possible. The same is true for node_state_diag.sh.
    if ((command_.contains("ecflow_client") || command_.contains("ecflow_ui_node_state_diag.sh") ||
         command_.contains("ecflow_ui_create_jira_issue.sh")) &&
        envHasToBeSet_) {
        QStringList pLst = {QString::fromStdString(DirectoryHandler::etcDir()),
                            QString::fromStdString(DirectoryHandler::exeDir())};
        QString extraPath;
        for (auto exeDir : pLst) {
            if (!exeDir.isEmpty()) {
                extraPath += exeDir + ":";
            }
        }

        if (!extraPath.isEmpty()) {
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            QString envPath         = env.value("PATH");
            env.insert("PATH", extraPath + envPath);
            proc_->setProcessEnvironment(env);
        }
    }

    connect(proc_, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(procFinished(int, QProcess::ExitStatus)));

    connect(proc_, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdOutput()));

    connect(proc_, SIGNAL(readyReadStandardError()), this, SLOT(slotStdError()));

    startTime_ = QDateTime::currentDateTime();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    proc_->start("/bin/sh", QStringList() << "-c" << command_);
#else
    proc_->start("/bin/sh -c \"" + command_ + "\"");
#endif
}

QString ShellCommand::command() const {
    return command_;
}

ShellCommand* ShellCommand::run(const std::string& cmd, const std::string& cmdDef, bool addToDialog) {
    return new ShellCommand(cmd, cmdDef, addToDialog);
}

void ShellCommand::procFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (!addToDialog_ && exitCode == 0 && exitStatus == QProcess::NormalExit) {
        return;
    }

    if (!item_) {
        item_ = CommandOutputHandler::instance()->addItem(
            command_, commandDef_, startTime_, CommandOutputHandler::NormalContext);
    }
    Q_ASSERT(item_);
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        CommandOutputHandler::instance()->finished(item_);
    }
    else {
        CommandOutputHandler::instance()->failed(item_);
    }

    deleteLater();
}

void ShellCommand::slotStdOutput() {
    if (!addToDialog_) {
        return;
    }

    if (!item_) {
        item_ = CommandOutputHandler::instance()->addItem(
            command_, commandDef_, startTime_, CommandOutputHandler::StdOutContext);
    }
    Q_ASSERT(item_);
    if (item_->isEnabled()) {
        QString txt = proc_->readAllStandardOutput();
        CommandOutputHandler::instance()->appendOutput(item_, txt);
    }
}

void ShellCommand::slotStdError() {
    if (!item_) {
        item_ = CommandOutputHandler::instance()->addItem(
            command_, commandDef_, startTime_, CommandOutputHandler::StdErrContext);
    }
    Q_ASSERT(item_);
    if (item_->isEnabled()) {
        QString txt = proc_->readAllStandardError();
        CommandOutputHandler::instance()->appendError(item_, txt);
    }
}
