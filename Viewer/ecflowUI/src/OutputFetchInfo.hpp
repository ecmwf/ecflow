/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
    ~OutputFetchInfo() override;

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
