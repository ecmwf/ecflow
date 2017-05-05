//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef DASHBOARDDOCK_HPP_
#define DASHBOARDDOCK_HPP_

#include <QDockWidget>

#include "ui_DashboardDockTitleWidget.h"

class DashboardWidget;
class QToolButton;

class DashboardDockTitleWidget : public QWidget, protected Ui::DashboardDockTitleWidget
{
Q_OBJECT

public:
	explicit DashboardDockTitleWidget(QWidget *parent=0);

    void addInfoPanelActions();
    QSize sizeHint() const;
	QSize minimumSizeHint() const;
	QToolButton* optionsTb() const;
    void setBcWidget(QWidget *w);
	void addActions(QList<QAction*> lst);
    void setDetachedAction(QAction *ac);
    void setMaximisedAction(QAction *ac);

public Q_SLOTS:
    void slotUpdateTitle(QString txt,QString type);

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
    QWidget* titleBc_;
    QPixmap warnPix_;
};

class DashboardDock : public QDockWidget
{
Q_OBJECT

public:
	explicit DashboardDock(DashboardWidget* dw,QWidget * parent=0);

Q_SIGNALS:
	void closeRequested();

protected:
	void showEvent(QShowEvent* event);
	void closeEvent (QCloseEvent *event);
};

#endif
