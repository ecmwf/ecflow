/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_FileWatcher_HPP
#define ecflow_viewer_FileWatcher_HPP

#include <QFile>
#include <QFileSystemWatcher>
#include <QStringList>

class FileWatcher : public QFileSystemWatcher {
    Q_OBJECT

public:
    FileWatcher(const std::string& filePath, qint64 offset, QObject* parent);

protected Q_SLOTS:
    void slotChanged(const QString& path);

Q_SIGNALS:
    void linesAppended(QStringList);

protected:
    QFile file_;
    qint64 offset_;
};

#endif /* ecflow_viewer_FileWatcher_HPP */
