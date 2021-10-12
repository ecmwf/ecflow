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
#include <QString>
#include <QMap>

#include "DirectoryHandler.hpp"
#include "UiLog.hpp"

static QMap<QProcess::ProcessError, QString> errorStr;

#define UI_FILETRANSFER_DEBUG_

VFileTransfer::VFileTransfer(QObject* parent) :
    QObject(parent),
    proc_(nullptr),
    ignoreSetX_(true),
    scriptName_("ecflow_ui_transfer_file.sh")
{
    if (errorStr.isEmpty()) {
        errorStr[QProcess::FailedToStart] = "failed to start";
        errorStr[QProcess::Crashed] = "crashed";
        errorStr[QProcess::Timedout] = "timed out";
        errorStr[QProcess::WriteError] = "failed with write error";
        errorStr[QProcess::ReadError] = "failed with read error";
        errorStr[QProcess::UnknownError] = "failed";
    }
}

bool VFileTransfer::isActive() const
{
//    UiLog().dbg() << "state=" << proc_->state();
    return (proc_ && proc_->state() != QProcess::NotRunning);
}

void VFileTransfer::transfer(QString sourceFile,QString host,QString targetFile,size_t lastBytes,
                             QString remoteUid)
{
    if(proc_)
    {
        proc_->kill();
        proc_->deleteLater();
        proc_=nullptr;
    }
    Q_ASSERT(proc_ == nullptr);

    if(!proc_)
    {
        proc_= new QProcess(this);

        connect(proc_,SIGNAL(finished(int,QProcess::ExitStatus)),
                this,SLOT(slotProcFinished(int,QProcess::ExitStatus)));

        connect(proc_,SIGNAL(readyReadStandardOutput()),
                this,SLOT(slotStdOutput()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        connect(proc_,SIGNAL(errorOccurred(QProcess::ProcessError)),
                this,SLOT(slotErrorOccurred(QProcess::ProcessError)));
#endif
    }

    Q_ASSERT(proc_->state() == QProcess::NotRunning);

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

    if(remoteUid.isEmpty() || remoteUid == "$USER")
       remoteUid = "__USER__";

    QString command=scriptName_ + " ";
    command+="\'" + sourceFile + "\' " + host + " \'" + targetFile + "\' " +  QString::number(lastBytes) +
            " " + remoteUid;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    proc_->start("/bin/sh", QStringList() << "-c" << command);
#else
    proc_->start("/bin/sh -c \"" + command + "\"");
#endif


#ifdef UI_FILETRANSFER_DEBUG_
    UiLog().dbg() << "exeDir=" << exeDir;
    UiLog().dbg() << "program=" << proc_->program();
    UiLog().dbg() << "arguments=" << proc_->arguments();
#endif
}

void VFileTransfer::stopTransfer()
{
    if(proc_) {
        proc_->kill();
        // on Mac OS (Big Sur) with Qt6 the slotProcFinished() is not
        // called automaticallywe we need to call it here!
        slotProcFinished(1, QProcess::CrashExit);
    }
}

void VFileTransfer::slotProcFinished(int exitCode,QProcess::ExitStatus exitStatus)
{
#ifdef UI_FILETRANSFER_DEBUG_
    UI_FUNCTION_LOG
#endif

    if(!proc_)
        return;

    QString stdOutTxt(proc_->readAllStandardOutput());
    QString stdErrTxt=stdErr();

#ifdef UI_FILETRANSFER_DEBUG_
    UiLog().dbg() << "exitCode=" << exitCode << "exitStatus=" << exitStatus;
    UiLog().dbg() << "errorString=" << proc_->errorString();
    UiLog().dbg() << "stdout=" << stdOutTxt;
    UiLog().dbg() << "stderr=" << stdErrTxt;
#endif

    QString errTxt=stdOutTxt;
    if(!errTxt.isEmpty())
    {
        errTxt+="/n";
    }
    errTxt+=stdErrTxt;
    QString errPreTxt="Script <b>" + scriptName_ + "</b> ";
    errPreTxt+=errorStr.value(proc_->error(),"failed") + "!";
    errPreTxt+=" Output:\n\n";
    errTxt.prepend(errPreTxt);

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

    if(proc_)
    {
        proc_->deleteLater();
        proc_=nullptr;
    }
}

// It is called when on MacOs with Qt6 and the process fails to start!
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
void VFileTransfer::slotErrorOccurred(QProcess::ProcessError e)
{
#ifdef UI_FILETRANSFER_DEBUG_
    UI_FUNCTION_LOG
    UiLog().dbg() << "errorString=" << proc_->errorString();
#endif
    slotProcFinished(1, QProcess::CrashExit);
}
#endif


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
