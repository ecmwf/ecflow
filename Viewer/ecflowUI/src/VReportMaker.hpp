/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
