/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_FileInfoLabel_HPP
#define ecflow_viewer_FileInfoLabel_HPP

#include <QDateTime>
#include <QLabel>

#include "VDir.hpp"
#include "VFile.hpp"

class VProperty;
class VReply;
class TimelineWidget;

class FileInfoLabel : public QLabel {
public:
    explicit FileInfoLabel(QWidget* parent = nullptr);

    void update(VReply*, QString str = QString());
    void update(VReply*, VFile_ptr, QString str = QString());
    void update(QString fullText, QString compactText);
    void clearIt();
    void setCompact(bool);
    static QString formatDate(QDateTime);
    static QString formatFileSize(QString, qint64 size);
    static QString formatKwPair(QString label, QString val);
    static QString formatKey(QString key);
    static QString formatHighlight(QString key);

protected:
    QString buildTooltipText();

    bool compact_{false};
    QString fullText_;
    QString compactText_;
    VProperty* showTransferDetailsProp_{nullptr};
};

class DirInfoLabel : public FileInfoLabel {
public:
    explicit DirInfoLabel(QWidget* parent = nullptr)
        : FileInfoLabel(parent) {}

    void update(VReply*);
};

#endif /* ecflow_viewer_FileInfoLabel_HPP */
