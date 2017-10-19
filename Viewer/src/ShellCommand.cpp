//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ShellCommand.hpp"

#include <QDebug>
#include <QProcess>
#include <QString>

#include <stdio.h>

#include "CommandOutputDialog.hpp"
#include "DirectoryHandler.hpp"
#include "CommandOutput.hpp"
#include "UiLog.hpp"
#include "VFile.hpp"

//static std::vector<ShellCommand*> commands;

bool ShellCommand::envChecked_=false;
bool ShellCommand::envHasToBeSet_=false;

ShellCommand::ShellCommand(const std::string& cmdStr,const std::string& cmdDefStr) :
    QObject(0),
    proc_(0),
    commandDef_(QString::fromStdString(cmdDefStr))
{
    QString cmdIn=QString::fromStdString(cmdStr);

    //A valid shell command must start with "sh "
    if(!cmdIn.startsWith("sh "))
    {
        deleteLater();
        return;
    }

    command_=cmdIn.mid(3);

    proc_=new QProcess(this);

    //Check environment - only has to be once
    if(!envChecked_)
    {
        envChecked_=true;

        QString exeDir=QString::fromStdString(DirectoryHandler::exeDir());

        //If there is an exe dir we check if it is added to the PATH env
        //variable
        if(!exeDir.isEmpty())
        {
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            QString envPath=env.value("PATH");
            if(!envPath.contains(exeDir))
                envHasToBeSet_=true;
        }
    }

    //If the shell command runs ecflow_client it has to be in the PATH env variable.
    //When we develop the ui it is not the case so we need to add its path to PATH
    //whenever is possible.
    if(command_.contains("ecflow_client") && envHasToBeSet_)
    {
        QString exeDir=QString::fromStdString(DirectoryHandler::exeDir());
        Q_ASSERT(!exeDir.isEmpty());
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString envPath=env.value("PATH");
        env.insert("PATH",exeDir + ":" + envPath);
        proc_->setProcessEnvironment(env);
    }

    connect(proc_,SIGNAL(finished(int,QProcess::ExitStatus)),
            this,SLOT(procFinished(int,QProcess::ExitStatus)));

    connect(proc_,SIGNAL(readyReadStandardOutput()),
            this,SLOT(slotStdOutput()));

    connect(proc_,SIGNAL(readyReadStandardError()),
            this,SLOT(slotStdError()));

    startTime_=QDateTime::currentDateTime();
    proc_->start("/bin/sh -c \"" + command_ + "\"");
}

QString ShellCommand::command() const
{
    return command_;
}

ShellCommand* ShellCommand::run(const std::string& cmd,const std::string& cmdDef)
{
    return new ShellCommand(cmd,cmdDef);
}

void ShellCommand::procFinished(int /*exitCode*/, QProcess::ExitStatus exitStatus)
{
    //UiLog().dbg() << "exit=" << exitCode << " status=" << exitStatus;
    if(!item_)
    {
        if(exitStatus == QProcess::NormalExit)
            CommandOutputHandler::instance()->finished(item_);
        else
            CommandOutputHandler::instance()->failed(item_);
    }
    deleteLater();

}

void ShellCommand::slotStdOutput()
{
    if(!item_)
    {
        item_=CommandOutputHandler::instance()->addItem(command_,commandDef_,startTime_);
    }
    Q_ASSERT(item_);
    QString txt=proc_->readAllStandardOutput();
    CommandOutputHandler::instance()->appendOutput(item_,txt);
}

void ShellCommand::slotStdError()
{
    if(!item_)
    {
        item_=CommandOutputHandler::instance()->addItem(command_,commandDef_,startTime_);
    }
    Q_ASSERT(item_);
    QString txt=proc_->readAllStandardError();
    //item_->appendError(txt);
    CommandOutputHandler::instance()->appendError(item_,txt);
}
