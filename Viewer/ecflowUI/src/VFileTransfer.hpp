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

#include <QDateTime>
#include <QProcess>

class VFileTransferCore : public QObject
{
    Q_OBJECT

public:
    enum ByteMode {AllBytes, BytesFromPos, LastBytes};
    VFileTransferCore(QObject* parent=nullptr);
    void transfer(QString sourceFile,QString host,QString targetFile,QString remoteUid, ByteMode byteMode, size_t byteVal);
    void transferLocalViaSocks(QString sourceFile, QString targetFile, ByteMode byteMode=AllBytes, size_t byteVal=0);

    void stopTransfer(bool broadcast);
    bool isActive() const;
    QString remoteUserAndHost() const {return remoteUserAndHost_;}
    static void socksRemoteUserAndHost(QString& user, QString& host);

protected Q_SLOTS:
    void slotProcFinished(int,QProcess::ExitStatus);
    void slotStdOutput();
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    void slotErrorOccurred(QProcess::ProcessError);
#endif

Q_SIGNALS:
    void transferFinished();
    void transferFailed(QString);
    void stdOutputAvailable(QString);

protected:
    void clear();
    virtual QString buildCommand(QString sourceFile, QString targetFile ,QString remoteUid, QString host,
                                        ByteMode byteMode, std::size_t byteVal) const = 0;
    QString stdErr();
    static QString buildSocksProxyJump();

    QProcess* proc_;
    QString targetFile_;
    bool ignoreSetX_;
    QString scriptName_;
    QString remoteUserAndHost_;
    bool byteMode_{AllBytes};
    size_t byteVal_{0};
};


class VFileTransfer: public VFileTransferCore
{
public:
    using  VFileTransferCore::VFileTransferCore;
protected:
    QString buildCommand(QString sourceFile, QString targetFile ,QString remoteUid, QString host,
                                        ByteMode byteMode, std::size_t byteVal) const override;
};

class VDirTransfer: public VFileTransferCore
{
public:
    using  VFileTransferCore::VFileTransferCore;
protected:
    QString buildCommand(QString sourceFile, QString targetFile ,QString remoteUid, QString host,
                                        ByteMode byteMode, std::size_t byteVal) const override;
};


#endif // VFILETRANSFER_HPP
