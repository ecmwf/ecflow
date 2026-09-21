// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QObject>
#include <QString>

class QTimer;

class LogTruncator : public QObject {
    Q_OBJECT
public:
    LogTruncator(QString, int, int, int, QObject* parent = nullptr);

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
