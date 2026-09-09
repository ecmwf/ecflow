/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TimelinePreLoadDialog_HPP
#define ecflow_viewer_TimelinePreLoadDialog_HPP

#include <QDialog>

class TimelineFileList;

namespace Ui {
class TimelinePreLoadDialog;
}

class TimelinePreLoadDialog : public QDialog {
public:
    explicit TimelinePreLoadDialog(QWidget* parent = nullptr);
    ~TimelinePreLoadDialog() override;

    void init(const TimelineFileList& lst);

protected:
    // void closeEvent(QCloseEvent * event);
    void readSettings();
    void writeSettings();

    Ui::TimelinePreLoadDialog* ui_;
};

#endif /* ecflow_viewer_TimelinePreLoadDialog_HPP */
