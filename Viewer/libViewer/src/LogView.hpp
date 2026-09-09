/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
