/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VReportMaker_HPP
#define ecflow_viewer_VReportMaker_HPP

#include <QObject>

#include "InfoPresenter.hpp"
#include "VFile.hpp"
#include "VInfo.hpp"

class OutputFileProvider;

class VReportMaker : public InfoPresenter, public QObject {
public:
    static void sendReport(VInfo_ptr);

    // From VInfoPresenter
    void infoReady(VReply*) override;
    void infoFailed(VReply*) override;

protected:
    explicit VReportMaker(QObject* parent = nullptr);
    void run(VInfo_ptr);
    void sendJiraReport(VFile_ptr file);
};

#endif /* ecflow_viewer_VReportMaker_HPP */
