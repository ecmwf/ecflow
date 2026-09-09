/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "FileWatcher.hpp"

FileWatcher::FileWatcher(const std::string& filePath, qint64 offset, QObject* parent)
    : QFileSystemWatcher(parent),
      offset_(offset) {
    connect(this, SIGNAL(fileChanged(QString)), this, SLOT(slotChanged(QString)));

    file_.setFileName(QString::fromStdString(filePath));
    if (!file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    file_.seek(offset_);

    addPath(file_.fileName());
}

void FileWatcher::slotChanged(const QString& path) {
    QStringList lst;
    if (path == file_.fileName()) {
        while (!file_.atEnd()) {
            lst << file_.readLine();
        }
    }

    Q_EMIT linesAppended(lst);
}
