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

#include "DirectoryHandler.hpp"
#include "UiLog.hpp"

VFileTransfer::VFileTransfer(QObject* parent) :
    QObject(parent),
    proc_(NULL),
    ignoreSetX_(true),
    scriptName_("ecflow_ui_transfer_file.sh")
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

    lastBytes=0;

    targetFile_=targetFile;

    //If there is an exe dir we check if it is added to the PATH env
    //variable
    QString exeDir=QString::fromStdString(DirectoryHandler::exeDir());
    if(!exeDir.isEmpty())
    {
      QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
      QString envPath=env.value("PATH");
      if(!envPath.contains(exeDir))
      {
          env.insert("PATH",exeDir + ":" + envPath);
          proc_->setProcessEnvironment(env);
      }
    }

    QString command=scriptName_ + " ";
    command+="\'" + sourceFile + "\' " + host + " \'" + targetFile + "\' " +  QString::number(lastBytes);

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

    QString stdOutTxt(proc_->readAllStandardOutput());
    QString stdErrTxt=stdErr();

    QString errTxt=stdOutTxt;
    if(!errTxt.isEmpty())
    {
        errTxt+="/n";
    }
    errTxt+=stdErrTxt;
    errTxt.prepend("Script <b>" + scriptName_ + "</b> failed!. Output:\n\n");

    if(exitCode == 0 && exitStatus == QProcess::NormalExit)
    {
        QFileInfo fi(targetFile_);
        if(fi.exists() && fi.size() >0)
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

QString VFileTransfer::stdErr()
{
    if(ignoreSetX_)
    {
        QString res;
        QString txt(proc_->readAllStandardError());
        Q_FOREACH(QString s,txt.split("\n"))
        {
            if(!s.startsWith("+ "))
                res+=s + "\n";
        }
        return res;
    }

    return QString(proc_->readAllStandardError());
}
