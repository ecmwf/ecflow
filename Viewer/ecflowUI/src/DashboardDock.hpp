/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_DashboardDock_HPP
#define ecflow_viewer_DashboardDock_HPP

#include <QDockWidget>

#include "ui_DashboardDockTitleWidget.h"

class DashboardWidget;
class QToolButton;

class DashboardDockTitleWidget : public QWidget, protected Ui::DashboardDockTitleWidget {
    Q_OBJECT

public:
    explicit DashboardDockTitleWidget(QWidget* parent = nullptr);

    void addInfoPanelActions();
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QToolButton* optionsTb() const;
    void setBcWidget(QWidget* w);
    void addActions(QList<QAction*> lst);
    void setDetachedAction(QAction* ac);
    void setMaximisedAction(QAction* ac);

public Q_SLOTS:
    void slotUpdateTitle(QString txt, QString type);

protected Q_SLOTS:
#if 0
    void on_floatTb__clicked(bool);
#endif
    void on_closeTb__clicked(bool);
    void slotActionChanged();

Q_SIGNALS:
    void detachedChanged(bool);

protected:
    QList<QToolButton*> actionTbList_;
    QWidget* titleBc_{nullptr};
    QPixmap warnPix_;
};

class DashboardDock : public QDockWidget {
    Q_OBJECT

public:
    explicit DashboardDock(DashboardWidget* dw, QWidget* parent = nullptr);

Q_SIGNALS:
    void closeRequested();
    ///
    /// @brief Emitted when the dock's dimensions change.
    ///
    /// Dock sizes are part of the dashboard's persisted dock state, so the
    /// owning dashboard uses this signal to schedule a session save.
    ///
    void layoutChanged();

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
};

#endif /* ecflow_viewer_DashboardDock_HPP */
