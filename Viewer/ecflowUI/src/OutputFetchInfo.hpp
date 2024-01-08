/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_OutputFetchInfo_HPP
#define ecflow_viewer_OutputFetchInfo_HPP

#include <string>
#include <vector>

#include <QWidget>

#include "VInfo.hpp"
#include "VReply.hpp"

class QAbstractButton;
class QButtonGroup;

namespace Ui {
class OutputFetchInfo;
}

class OutputFetchInfo : public QWidget {
    Q_OBJECT
public:
    explicit OutputFetchInfo(QWidget* parent);
    void clearInfo();
    void setInfo(VReply*, VInfo_ptr info = nullptr);
    void setError(QString);
    void setError(const std::vector<std::string>& errorVec);

protected Q_SLOTS:
    void buttonClicked(QAbstractButton*);

protected:
    virtual QString makeHtml(VReply*, VInfo_ptr info) = 0;
    QString buildList(QStringList, bool ordered = false);
    QString formatErrors(const std::vector<std::string>& errorVec) const;
    void parseTry(QString s, QString& path, QString& msg);

    Ui::OutputFetchInfo* ui_{nullptr};
    QButtonGroup* bGroup_{nullptr};
};

class OutputFileFetchInfo : public OutputFetchInfo {
public:
    using OutputFetchInfo::OutputFetchInfo;

protected:
    QString makeHtml(VReply*, VInfo_ptr info) override;
};

class OutputDirFetchInfo : public OutputFetchInfo {
public:
    using OutputFetchInfo::OutputFetchInfo;

protected:
    QString makeHtml(VReply*, VInfo_ptr info) override;
    QString makeSearchPath(QString path) const;
};

#endif /* ecflow_viewer_OutputFetchInfo_HPP */
