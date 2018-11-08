//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VFileTransfer.hpp"

#include <QFileInfo>
#include <QProcess>

VFileTransfer::VFileTransfer(QObject* parent) :
    QObject(parent)
{
    proc_= new QProcess(this);

    connect(proc_,SIGNAL(finished(int,QProcess::ExitStatus)),
                this,SLOT(slotProcFinished(int,QProcess::ExitStatus)));

    connect(proc_,SIGNAL(readyReadStandardOutput()),
            this,SLOT(slotStdOutput()));
}

void VFileTransfer::transfer(QString sourceFile,QString host,QString targetFile)
{
    if(proc_->state() != QProcess::NotRunning)
        proc_->kill();

    targetFile_=targetFile;

    QString command="scp " + host + ":/" + sourceFile + " " + targetFile_;
    proc_->start("/bin/sh -c \"" + command + "\"");
}

void VFileTransfer::stopTransfer()
{
    proc_->kill();
}

void VFileTransfer::slotProcFinished(int exitCode,QProcess::ExitStatus exitStatus)
{
    bool failed=false;
    if(exitCode == 0 && exitStatus == QProcess::NormalExit)
    {
        QFileInfo fi(targetFile_);
        if(fi.exists())
        {
            Q_EMIT transferFinished();
            return;
        }
    }

    Q_EMIT transferFailed(QString(proc_->readAllStandardError()));
}

void VFileTransfer::slotStdOutput()
{
    Q_EMIT stdOutputAvailable(proc_->readAllStandardOutput());
}

