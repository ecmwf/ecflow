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

class VFileTransfer : public QObject
{
    Q_OBJECT

public:
    enum ByteMode {AllBytes, BytesFromPos, LastBytes};
    VFileTransfer(QObject* parent=nullptr);
    void transfer(QString sourceFile,QString host,QString targetFile,ByteMode byteMode, size_t byteVal, QString remoteUid={});
    void transferLocalViaSocks(QString sourceFile, QString targetFile, ByteMode byteMode, size_t byteVal);

    void stopTransfer(bool broadcast);
    bool isActive() const;

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
    QString stdErr();
    static QString buildSocksProxyJump();
    static void socksRemoteUserAndHost(QString& user, QString& host);

    QProcess* proc_;
    QString targetFile_;
    bool ignoreSetX_;
    QString scriptName_;
};

#endif // VFILETRANSFER_HPP
