//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TIMELINEPRELOADDIALOG_HPP
#define TIMELINEPRELOADDIALOG_HPP

#include <QDialog>

class TimelineFileList;

namespace Ui {
    class TimelinePreLoadDialog;
}

class TimelinePreLoadDialog : public QDialog
{
public:
    explicit TimelinePreLoadDialog(QWidget *parent=0);
    ~TimelinePreLoadDialog();

    void init(const TimelineFileList& lst);

protected:
    //void closeEvent(QCloseEvent * event);
    void readSettings();
    void writeSettings();

    Ui::TimelinePreLoadDialog* ui_;
};

#endif // TIMELINEPRELOADDIALOG_HPP
