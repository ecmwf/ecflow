//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VFILETRANSFER_HPP
#define VFILETRANSFER_HPP

#include <string>

#include <QDateTime>
#include <QProcess>
#include <QElapsedTimer>

#include "VFile.hpp"


class VFileTransferCore : public QObject
{
    Q_OBJECT

public:
    VFileTransferCore(QObject* parent=nullptr);
    virtual ~VFileTransferCore();

    virtual void clear();
    void stopTransfer(bool broadcast);
    bool isActive() const;
    static QString socksPort();
    static QString socksUser();
    VFile_ptr const result() {return fResult_;}

protected Q_SLOTS:
    void slotProcFinished(int,QProcess::ExitStatus);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    void slotErrorOccurred(QProcess::ProcessError);
#endif

Q_SIGNALS:
    void transferFinished();
    void transferFailed(QString);
    void stdOutputAvailable(QString);

protected:
    void stopTransfer(bool broadcast, bool doClear);
    void transferIt();
    virtual QString buildCommand() = 0;
    QString stdErr();
    virtual bool checkResults()=0;
    static void socksRemoteUserAndHost(QString& user, QString& host);
    static QString buildSocksProxyJump();

    QProcess* proc_{nullptr};
    QString sourceFile_;
    VFile_ptr fResult_;
    bool ignoreSetX_{false};
    QString scriptName_;
    QString remoteHost_;
    QString remoteUser_;
    QString remoteUserAndHost_;
    QString socksPort_;
    bool useSocks_{false};
    unsigned int transferDuration_{0};
    QElapsedTimer stopper_;
};


class VFileTransfer: public VFileTransferCore
{
public:
    enum ByteMode {AllBytes, BytesFromPos, LastBytes};
    using  VFileTransferCore::VFileTransferCore;

    void clear() override;
    void transfer(QString sourceFile,QString host,QString remoteUid, ByteMode byteMode, size_t byteVal);
    void transferLocalViaSocks(QString sourceFile, ByteMode byteMode, size_t byteVal);
    void transferLocalViaSocks(QString sourceFile, ByteMode byteMode, size_t byteVal, size_t remoteModTime, const std::string& remoteCheckSum);

protected:
    void transferLocalViaSocks(QString sourceFile, ByteMode byteMode, size_t byteVal, bool useMetaData, size_t remoteModTime, const std::string& remoteCheckSum);
    QString buildCommand() override;
    bool checkResults() override;
    bool readMetaData();

    ByteMode byteMode_{AllBytes};
    size_t byteVal_{0};
    bool useMetaData_{false};
    VFile_ptr fMetaRes_;
    size_t modTime_{0};
    std::string checkSum_;
};

class VDirTransfer: public VFileTransferCore
{
public:
    using  VFileTransferCore::VFileTransferCore;
    void transfer(QString sourceFile,QString host,QString remoteUid);
    void transferLocalViaSocks(QString sourceFile);

protected:
    QString buildCommand() override;
    bool checkResults() override;
};

#endif // VFILETRANSFER_HPP
