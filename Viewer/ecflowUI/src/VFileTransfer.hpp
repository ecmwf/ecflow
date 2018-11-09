//============================================================================
// Copyright 2009-2018 ECMWF.
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
    VFileTransfer(QObject* parent=0);
    void transfer(QString sourceFile,QString host,QString targetFile,size_t lastBytes);
    void stopTransfer();

protected Q_SLOTS:
    void slotProcFinished(int,QProcess::ExitStatus);
    void slotStdOutput();

Q_SIGNALS:
    void transferFinished();
    void transferFailed(QString);
    void stdOutputAvailable(QString);

protected:
    QProcess* proc_;
    QString targetFile_;
};

#endif // VFILETRANSFER_HPP
