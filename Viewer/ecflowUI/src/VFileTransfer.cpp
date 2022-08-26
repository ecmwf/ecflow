//============================================================================
// Copyright 2009- ECMWF.
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
#include "VConfig.hpp"
#include "VProperty.hpp"

static QMap<QProcess::ProcessError, QString> errorStr;

#define UI_FILETRANSFER_DEBUG_

VFileTransferCore::VFileTransferCore(QObject* parent) :
    QObject(parent),
    proc_(nullptr),
    ignoreSetX_(false),
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

bool VFileTransferCore::isActive() const
{
//    UiLog().dbg() << "state=" << proc_->state();
    return (proc_ && proc_->state() != QProcess::NotRunning);
}

void VFileTransferCore::clear()
{
    remoteUserAndHost_.clear();
    targetFile_.clear();
    byteMode_=AllBytes;
    byteVal_=0;
    transferDuration_ = 0;
}

void VFileTransferCore::transferLocalViaSocks(QString sourceFile, QString targetFile, ByteMode byteMode, size_t byteVal)
{
    QString user, host;
    socksRemoteUserAndHost(user, host);
    transfer(sourceFile,host,targetFile,user,byteMode,byteVal);
}

void VFileTransferCore::transfer(QString sourceFile,QString host,QString targetFile, QString remoteUid, ByteMode byteMode,
                             size_t byteVal)
{
    clear();
    if(proc_)
    {
        proc_->disconnect(this);
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

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        connect(proc_,SIGNAL(errorOccurred(QProcess::ProcessError)),
                this,SLOT(slotErrorOccurred(QProcess::ProcessError)));
#endif
    }

    Q_ASSERT(proc_->state() == QProcess::NotRunning);

    targetFile_=targetFile;
    byteMode_ = byteMode;
    byteVal_ = byteVal;

    //If there is an exe dir we check if it is added to the PATH env
    //variable.
    QString exeDir=QString::fromStdString(DirectoryHandler::etcDir());
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

    remoteUserAndHost_ = remoteUid + "@" + host;

    QString command = buildCommand(sourceFile, targetFile, remoteUid, host, byteMode, byteVal);

    UiLog().dbg() << "command=" << command;

    stopper_.start();

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

void VFileTransferCore::stopTransfer(bool broadcast)
{
    if(proc_) {
        proc_->kill();
        if (broadcast) {
            // on Mac OS (Big Sur) with Qt6 the slotProcFinished() is not
            // called automatically so we need to call it here!
            slotProcFinished(1, QProcess::CrashExit);
        } else {
            if(proc_) {
                proc_->disconnect(this);
                proc_->deleteLater();
                proc_=nullptr;
            }
        }
        clear();
    }
}

void VFileTransferCore::slotProcFinished(int exitCode,QProcess::ExitStatus exitStatus)
{
#ifdef UI_FILETRANSFER_DEBUG_
    UI_FUNCTION_LOG
#endif

    if(!proc_)
        return;

    transferDuration_ = stopper_.elapsed();

    QString stdErrTxt=stdErr();
    QString stdOutTxt(proc_->readAllStandardOutput());

    stdErrTxt = stdErrTxt.trimmed();
    stdOutTxt = stdOutTxt.trimmed();

#ifdef UI_FILETRANSFER_DEBUG_
    UiLog().dbg() << "exitCode=" << exitCode << " exitStatus=" << exitStatus;
    UiLog().dbg() << "errorString=" << proc_->errorString();
    UiLog().dbg() << "stdout=" << stdOutTxt;
    UiLog().dbg() << "stderr=" << stdErrTxt;
#endif

    QString errTxt=stdOutTxt;
    if(!errTxt.isEmpty())
    {
        errTxt+="\n\n";
    }
    errTxt+=stdErrTxt;
    QString errPreTxt="Script <b>" + scriptName_ + "</b> ";
    errPreTxt+=errorStr.value(proc_->error(),"failed") + "!\n\n";
    errTxt.prepend(errPreTxt);

    if(exitCode == 0 && exitStatus == QProcess::NormalExit)
    {
        QFileInfo fi(targetFile_);
        if((fi.exists() && fi.size() >0) || (byteMode_ != AllBytes && byteVal_ != 0))
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
void VFileTransferCore::slotErrorOccurred(QProcess::ProcessError e)
{
#ifdef UI_FILETRANSFER_DEBUG_
    UI_FUNCTION_LOG
    if (proc_) {
        UiLog().dbg() << "errorString=" << proc_->errorString();
    }
#endif
    slotProcFinished(1, QProcess::CrashExit);
}
#endif

QString VFileTransferCore::stdErr()
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

QString VFileTransferCore::buildSocksProxyJump()
{
    QString s;
    if (VConfig::instance()->proxychainsUsed()) {
        auto p1 = VConfig::instance()->find("network.sshJump.proxyJumpUser");
        auto p2 = VConfig::instance()->find("network.sshJump.proxyJumpHost");
        if (p1 && p2) {
            QString user = p1->valueAsString();
            if (user.isEmpty() || user == "$USER") {
                if (char* ch = getenv("USER")) {
                    user = QString(ch);
                }
            }
            s = user + "@" + p2->valueAsString();
        }
    }
    return s;
}

void VFileTransferCore::socksRemoteUserAndHost(QString& user, QString& host)
{
    user.clear();
    host.clear();
    if (VConfig::instance()->proxychainsUsed()) {
        auto p1 = VConfig::instance()->find("network.socks.remoteUser");
        auto p2 = VConfig::instance()->find("network.socks.remoteHost");
        if (p1 && p2) {
            user = p1->valueAsString();
            if (user.isEmpty() || user == "$USER") {
                if (char* ch = getenv("USER")) {
                    user = QString(ch);
                }
            }
            host = p2->valueAsString();
        }
    }
}

QString VFileTransfer::buildCommand(QString sourceFile, QString targetFile, QString remoteUid, QString host,
                                    ByteMode byteMode, std::size_t byteVal) const
{
    QString byteModeStr = "all";
    if (byteMode == BytesFromPos) {
        byteModeStr = "pos";
    } else if (byteMode == LastBytes) {
        byteModeStr = "last";
    }

    QString proxyJump = buildSocksProxyJump();

    QString command=scriptName_ + " -m file ";
    command += "-s \'" + sourceFile + "\' -u " + remoteUid + " -h " + host +
            " -t " + " \'" + targetFile + "\' -j " + proxyJump +
            " -b " + byteModeStr + " -v " +  QString::number(byteVal);

    return command;
}

QString VDirTransfer::buildCommand(QString sourceFile, QString targetFile, QString remoteUid, QString host, ByteMode /*byteMode*/, std::size_t /*byteVal*/) const
{
    QString proxyJump = buildSocksProxyJump();

    QString command=scriptName_ + " -m dir ";
    command += "-s \'" + sourceFile + "\' -u " + remoteUid + " -h " + host +
            " -t " + " \'" + targetFile + "\' -j " + proxyJump;

    return command;
}
