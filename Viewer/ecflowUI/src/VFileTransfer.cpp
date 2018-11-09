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

#include <QtGlobal>
#include <QFileInfo>
#include <QProcess>

#include "UiLog.hpp"

VFileTransfer::VFileTransfer(QObject* parent) :
    QObject(parent),
    proc_(NULL)
{
}

void VFileTransfer::transfer(QString sourceFile,QString host,QString targetFile,size_t lastBytes)
{
    if(proc_)
    {
        proc_->kill();
        proc_->deleteLater();
        proc_=NULL;
    }
    Q_ASSERT(proc_ == NULL);

    if(!proc_)
    {
        proc_= new QProcess(this);

        connect(proc_,SIGNAL(finished(int,QProcess::ExitStatus)),
                this,SLOT(slotProcFinished(int,QProcess::ExitStatus)));

        connect(proc_,SIGNAL(readyReadStandardOutput()),
                this,SLOT(slotStdOutput()));
    }

    Q_ASSERT(proc_->state() == QProcess::NotRunning);

    targetFile_=targetFile;

    QString userId = qgetenv("USER");
    if (userId.isEmpty())
        userId = qgetenv("USERNAME");

    QString command;
    //fetch the lastBytes
    if(lastBytes != 0)
    {
        command="ssh " + userId  + "@" + host +
                " \'tail -c " + QString::number(lastBytes) + " " + sourceFile + "\'"; // > " + targetFile;
        proc_->setStandardOutputFile(targetFile_);
    }
    //fetch the whole file
    else
    {
        command="scp " + host + ":/" + sourceFile + " " + targetFile_;
        //with script we will write the progress into the stdout
        command="script -q -c \'" + command + "\'";
    }

    proc_->start("/bin/sh -c \"" + command + "\"");
    //UiLog().dbg() << "/bin/sh -c \"" + command + "\"";
}

void VFileTransfer::stopTransfer()
{
    if(proc_)
        proc_->kill();
}

void VFileTransfer::slotProcFinished(int exitCode,QProcess::ExitStatus exitStatus)
{
    if(!proc_)
        return;

    bool failed=false;
    QString errTxt=QString(proc_->readAllStandardOutput()) + " " + QString(proc_->readAllStandardError());
    if(exitCode == 0 && exitStatus == QProcess::NormalExit)
    {
        QFileInfo fi(targetFile_);
        if(fi.exists())
        {
            Q_EMIT transferFinished();
            return;
        }
        else
        {
           Q_EMIT transferFailed(errTxt);
        }
    }

    Q_EMIT transferFailed(errTxt);
}

void VFileTransfer::slotStdOutput()
{
    if(!proc_)
        return;

    Q_EMIT stdOutputAvailable(proc_->readAllStandardOutput());
}

