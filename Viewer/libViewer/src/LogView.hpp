/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_LogViewer_HPP
#define ecflow_viewer_LogViewer_HPP

#include <QTreeView>

class LogModel;

class LogView : public QTreeView {
    Q_OBJECT
public:
    explicit LogView(QWidget* parent = nullptr);
    void setLogModel(LogModel*);
    void setModel(QAbstractItemModel*) override;

public Q_SLOTS:
    // void setHighlightPeriod(qint64,qint64,qint64);
    void scrolltToHighlightPeriod();
    void rerender();

protected:
    LogModel* logModel_{nullptr};
};

#endif /* ecflow_viewer_LogViewer_HPP */
