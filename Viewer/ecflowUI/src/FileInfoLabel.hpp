//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef FILEINFOLABEL_HPP_
#define FILEINFOLABEL_HPP_

#include <QDateTime>
#include <QLabel>

#include "VFile.hpp"
#include "VDir.hpp"

class VProperty;
class VReply;
class TimelineWidget;

class FileInfoLabel : public QLabel
{
public:
	explicit FileInfoLabel(QWidget* parent=nullptr);

    void update(VReply*,QString str=QString());
    void update(VReply*,VFile_ptr, QString str=QString());
    void update(QString fullText, QString compactText);
    void clearIt();
    void setCompact(bool);
    static QString formatDate(QDateTime);
    static QString formatFileSize(QString,qint64 size);
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

class DirInfoLabel : public FileInfoLabel
{
public:
	explicit DirInfoLabel(QWidget* parent=nullptr) : FileInfoLabel(parent) {}

    void update(VReply*);

};

#endif
