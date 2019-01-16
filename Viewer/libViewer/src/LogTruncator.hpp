//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef LOGTRUNCATOR_HPP
#define LOGTRUNCATOR_HPP

#include <QObject>
#include <QString>

class QTimer;

class LogTruncator : public QObject
{
    Q_OBJECT
public:
    LogTruncator(QString, int,int,int,QObject* parent=nullptr);

protected Q_SLOTS:
    void truncate();

Q_SIGNALS:
    void truncateBegin();
    void truncateEnd();

private:
    QString path_;
    QTimer* timer_;
    int timeout_;
    int sizeLimit_;
    int lineNum_;
};

#endif // LOGTRUNCATOR_HPP

