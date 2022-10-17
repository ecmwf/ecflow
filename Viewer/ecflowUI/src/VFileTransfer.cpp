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

//#define UI_FILETRANSFER_DEBUG_

VFileTransferCore::VFileTransferCore(QObject* parent) :
    QObject(parent),
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

VFileTransferCore::~VFileTransferCore()
= default;

bool VFileTransferCore::isActive() const
{
    return (proc_ && proc_->state() != QProcess::NotRunning);
}

void VFileTransferCore::clear()
{
    if (isActive()) {
        stopTransfer(false, false);
    }

    remoteUser_.clear();
    remoteHost_.clear();
    remoteUserAndHost_.clear();
    sourceFile_.clear();
    fResult_.reset();
    transferDuration_ = 0;
    socksPort_.clear();
    useSocks_ = false;
}

void VFileTransferCore::stopTransfer(bool broadcast)
{
    stopTransfer(broadcast, true);
}

// Use doClear carefully to avoid infinite recurso
void VFileTransferCore::stopTransfer(bool broadcast, bool doClear)
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
        if (doClear) {
            clear();
        }
    }
}

void VFileTransferCore::transferIt()
{
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


    fResult_ = VFile::createTmpFile(true); //we will delete the file from disk

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
/*
    if(remoteUid.isEmpty() || remoteUid == "$USER")
       remoteUid = "__USER__";

    remoteUserAndHost_ = remoteUid + "@" + host*/;

    QString command = buildCommand();

#ifdef UI_FILETRANSFER_DEBUG_
    UiLog().dbg() << UI_FN_INFO;
    UiLog().dbg() << " command=" << command;
    UiLog().dbg() << " exeDir=" << exeDir;
    UiLog().dbg() << " program=" << proc_->program();
    UiLog().dbg() << " arguments=" << proc_->arguments();
#endif

    stopper_.start();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    proc_->start("/bin/sh", QStringList() << "-c" << command);
#else
    proc_->start("/bin/sh -c \"" + command + "\"");
#endif
}

void VFileTransferCore::slotProcFinished(int exitCode,QProcess::ExitStatus exitStatus)
{
#ifdef UI_FILETRANSFER_DEBUG_
    UI_FN_DBG
#endif

    if(!proc_)
        return;

    transferDuration_ = stopper_.elapsed();

    QString stdErrTxt=stdErr();
    QString stdOutTxt(proc_->readAllStandardOutput());

    stdErrTxt = stdErrTxt.trimmed();
    stdOutTxt = stdOutTxt.trimmed();

#ifdef UI_FILETRANSFER_DEBUG_
    UiLog().dbg() << " exitCode=" << exitCode << " exitStatus=" << exitStatus;
    UiLog().dbg() << " errorString=" << proc_->errorString();
    UiLog().dbg() << " stdout=" << stdOutTxt;
    UiLog().dbg() << " stderr=" << stdErrTxt;
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

    if(exitCode == 0 && exitStatus == QProcess::NormalExit) {
        if (checkResults()) {            
            Q_EMIT transferFinished();
            return;
        } else
        {
           Q_EMIT transferFailed(errTxt);
        }
    }

    Q_EMIT transferFailed(errTxt);

    if(proc_) {
        proc_->deleteLater();
        proc_=nullptr;
    }
}

// It is called when on MacOs with Qt6 and the process fails to start!
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
void VFileTransferCore::slotErrorOccurred(QProcess::ProcessError)
{   
#ifdef UI_FILETRANSFER_DEBUG_
    UI_FN_DBG
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
#if 0
    if (VConfig::instance()->proxychainsUsed()) {
        auto p1 = VConfig::instance()->find("network.sshJump.proxyJumpUser");
        auto p2 = VConfig::instance()->find("network.sshJump.proxyJumpHost");
        if (p1 && p2) {
            QString user = p1->valueAsString().simplified();
            QString host = p2->valueAsString().simplified();
            if (!host.isEmpty()) {
                if (user == "$USER") {
                    if (char* ch = getenv("USER")) {
                        user = QString(ch);
                    }
                }
                s = user + "@" + p2->valueAsString();
            }
        }
    }
#endif
    return s;
}

QString VFileTransferCore::socksPort()
{
    if (VConfig::instance()->proxychainsUsed()) {
        auto p = VConfig::instance()->find("network.socks.port");
        if (p) {
            return p->valueAsString();
        }
    }
    return {};
}

void VFileTransferCore::socksRemoteUserAndHost(QString& user, QString& host)
{
    user.clear();
    host.clear();
#if 0
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
#endif
}

//============================================
//
// VFileTransfer
//
//============================================

void VFileTransfer::clear()
{
    VFileTransferCore::clear();
    byteMode_=AllBytes;
    byteVal_=0;
    useMetaData_=false;
    fMetaRes_.reset();
    modTime_=0;
    checkSum_.clear();
}

void VFileTransfer::transferLocalViaSocks(QString sourceFile, ByteMode byteMode, size_t byteVal)
{
    transferLocalViaSocks(sourceFile, byteMode, byteVal, false, 0, {});
}

void VFileTransfer::transferLocalViaSocks(QString sourceFile, ByteMode byteMode, size_t byteVal,
                                           size_t remoteModTime, const std::string& remoteCheckSum)
{
    transferLocalViaSocks(sourceFile,byteMode,byteVal,true,remoteModTime, remoteCheckSum);

}

void VFileTransfer::transferLocalViaSocks(QString sourceFile, ByteMode byteMode, size_t byteVal, bool useMetaData,
                                           size_t remoteModTime, const std::string& remoteCheckSum)
{
    clear();

    sourceFile_ = sourceFile;
    byteMode_ = byteMode;
    byteVal_ = byteVal;
    useMetaData_ = useMetaData;
    modTime_ = remoteModTime;
    checkSum_ = remoteCheckSum;
    socksPort_ = socksPort();
    useSocks_ = true;

    transferIt();
}

void VFileTransfer::transfer(QString sourceFile,QString host, QString remoteUid, ByteMode byteMode,
                             size_t byteVal)
{
    clear();

    remoteUser_ = remoteUid;
    remoteHost_ = host;
    sourceFile_ = sourceFile;
    byteMode_ = byteMode;
    byteVal_ = byteVal;
    useMetaData_ = false;
    useSocks_ = false;

    transferIt();
}

QString VFileTransfer::buildCommand()
{
    QString byteModeStr = "all";
    if (byteMode_ == BytesFromPos) {
        byteModeStr = "pos";
    } else if (byteMode_ == LastBytes) {
        byteModeStr = "last";
    }

    QString proxyJump = buildSocksProxyJump();

    QString command=scriptName_;

    if (useMetaData_) {
        command += " -m file_md ";
    } else {
        command += " -m file ";
    }

    auto targetFile = QString::fromStdString(fResult_->path());

    command += "-s \'" + sourceFile_ +
                "\' -t " + " \'" + targetFile + "\' -b " + byteModeStr + " -v " +  QString::number(byteVal_);

    if (proxyJump != "__NOJUMP__" && !proxyJump.isEmpty()) {
        command += " -j " + proxyJump;
    }

    if (useSocks_) {
        command += " -p " + socksPort_;
    } else {
        auto u = remoteUser_;
        if(u.isEmpty() || u == "$USER") {
           u = "__USER__";
        }
        command += " -u " + u + " -h " + remoteHost_;
    }


//    command += "-s \'" + sourceFile_ + "\' -u " + remoteUid + " -h " + host +
//            " -t " + " \'" + targetFile + "\' -j " + proxyJump +
//            " -b " + byteModeStr + " -v " +  QString::number(byteVal_);

    if (useMetaData_) {
        fMetaRes_ =  VFile::createTmpFile(true); //we will delete the file from disk
        Q_ASSERT(fMetaRes_);
        command += " -x \'" + QString::fromStdString(fMetaRes_->path()) +
                   "\'  -o " + QString::number(modTime_) +
                   " -c " + ((!checkSum_.empty())?QString::fromStdString(checkSum_):"x");
    }

    return command;
}

bool VFileTransfer::checkResults()
{
    if (!fResult_) {
        return false;
    }

    if (useMetaData_) {
        // this sets the delta contents
        if (!readMetaData()) {
            return false;
        }
    } else {
        fResult_->setDeltaContents(byteMode_==BytesFromPos);
    }

    fResult_->setSourcePath(sourceFile_.toStdString());
    fResult_->setFetchMode(VFile::TransferFetchMode);
    std::string method;
    if (!remoteUserAndHost_.isEmpty()) {
        method = "from " + remoteUserAndHost_.toStdString() + " via ssh";
    } else {
        method ="via ssh";
    }
    fResult_->setFetchModeStr(method);
    fResult_->setTransferDuration(transferDuration_);
    fResult_->setFetchDate(QDateTime::currentDateTime());

#ifdef UI_FILETRANSFER_DEBUG_
    if (fResult) {
        UiLog().dbg() << UI_FN_INFO << "result is null";
    } else {
        UiLog().dbg() << UI_FN_INFO << "result size=" << fResult_->sizeInBytes();
    }
#endif
    return (!fResult_->isEmpty() || fResult_->hasDeltaContents() || (byteMode_ == LastBytes && byteVal_ != 0));
}

bool VFileTransfer::readMetaData()
{
    //  format:
    //      retCode:modTime:checkSum:padding
    //
    //      where:
    //          - retCode is a 1-digit number
    //          - modeTime/checkSum can be empty
    //          - padding can be empty
    //      note:
    //          - in AllBytes mode the retCode can only be "'0"
    //          - in delta (=BytesFromPos) mode:
    //                retCode = "0" : delta transferred
    //                retCode = "1" : the whole file transferred
    //          - in delta mode a single "1" message is allowed
    //
    //  e.g.:
    //      0:1662369142:8fssaf:abcd
    //      0:1662369142::abcd
    //      1:::
    //      0
    if (useMetaData_) {
        if (!fMetaRes_) {
            return false;
        }
        QFile f(fMetaRes_->path().c_str());
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            QString ds = f.readAll();
            ds = ds.simplified();
#ifdef UI_FILETRANSFER_DEBUG_
            UiLog().dbg() << UI_FN_INFO << "ds=" << ds;
#endif
            if (byteMode_ == BytesFromPos && ds == "1") {
                fResult_->setDeltaContents(true);
                fResult_->setSourceModTime(modTime_);
                fResult_->setSourceCheckSum(checkSum_);
                return true;
            }

            auto lst = ds.split(":");
#ifdef UI_FILETRANSFER_DEBUG_
            UiLog().dbg() << " lst=" << lst;
#endif
            if (lst.count() >=  3) {
                if (byteMode_ == AllBytes) {
                    fResult_->setDeltaContents(false);
                    if (lst[0] != "0") {
                        return false;
                    }
                } else if (byteMode_ == BytesFromPos) {
                    // all file
                    if (lst[0] == "0") {
                        fResult_->setDeltaContents(false);
                    // delta
                    } else if (lst[0] == "1") {
                        fResult_->setDeltaContents(true);
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
                fResult_->setSourceModTime(lst[1].toUInt());
                fResult_->setSourceCheckSum(lst[2].toStdString());
                return true;
            }

        }
        return false;
    }
    return true;
}

//============================================
//
// VDirTransfer
//
//============================================

void VDirTransfer::transfer(QString sourceFile,QString host,QString remoteUid)
{
    clear();

    remoteHost_ = host;
    remoteUser_ = remoteUid;
    sourceFile_ = sourceFile;

    transferIt();
}


void VDirTransfer::transferLocalViaSocks(QString sourceFile)
{
    clear();

    sourceFile_ = sourceFile;
    socksPort_ = socksPort();
    useSocks_ = true;

    transferIt();
}

QString VDirTransfer::buildCommand()
{
    QString proxyJump = buildSocksProxyJump();

    auto targetFile = QString::fromStdString(fResult_->path());

    QString command = scriptName_ + " -m dir -s \'" + sourceFile_ + "\' -t " + " \'" + targetFile + "\' ";

    if (proxyJump != "__NOJUMP__" && !proxyJump.isEmpty()) {
        command += " -j " + proxyJump;
    }

    if (useSocks_) {
        command += " -p " + socksPort_;
    } else {
        auto u = remoteUser_;
        if(u.isEmpty() || u == "$USER") {
           u = "__USER__";
        }
        command += " -u " + u + " -h " + remoteHost_;
    }

    return command;
}

bool VDirTransfer::checkResults()
{
    if (fResult_) {
        fResult_->setFetchMode(VFile::TransferFetchMode);
        std::string method;
        if (!remoteUserAndHost_.isEmpty()) {
            method = "from " + remoteUserAndHost_.toStdString() + " via ssh";
        } else {
            method ="via ssh";
        }
        fResult_->setFetchModeStr(method);
        fResult_->setTransferDuration(transferDuration_);
        fResult_->setFetchDate(QDateTime::currentDateTime());
        return (!fResult_->isEmpty());
    }
    return false;
}
