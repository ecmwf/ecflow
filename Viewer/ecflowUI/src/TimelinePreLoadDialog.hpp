// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
