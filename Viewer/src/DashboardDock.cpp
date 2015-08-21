//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "DashboardDock.hpp"

#include <QPalette>
#include <QPainter>
#include <QPixmap>

#include "DashboardWidget.hpp"

DashboardDockTitleWidget::DashboardDockTitleWidget(QWidget *parent) :
		QWidget(parent)
{
	setupUi(this);

	/*QPalette p=palette();
	p.setColor(QPalette::Window,QColor(230,230,230));
	setPalette(p);*/

/*
	QPixmap pix(8,8);
	pix.fill(Qt::transparent);

	QPainter painter(&pix);

	painter.setRenderHints(QPainter::Antialiasing,true);
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(179,179,179));
	painter.drawEllipse(QPoint(3,3),1,1);
	painter.drawEllipse(QPoint(6,6),1,1);

	QBrush b(pix);

	QPalette p=leftLabel_->palette();
	p.setBrush(QPalette::Window,b);
	leftLabel_->setPalette(p);

	p=rightLabel_->palette();
	p.setBrush(QPalette::Window,b);
	rightLabel_->setPalette(p);*/
}

QSize DashboardDockTitleWidget::sizeHint() const
{
	QSize s=QWidget::sizeHint();
	//s.setHeight();
	return s;
}

QSize DashboardDockTitleWidget::minimumSizeHint() const
{
	QSize s=QWidget::minimumSizeHint();
	//s.setHeight(16);
	return s;
}

QToolButton* DashboardDockTitleWidget::optionsTb() const
{
	return optionsTb_;
}

void DashboardDockTitleWidget::on_floatTb__clicked(bool)
{
	if(QDockWidget *dw = qobject_cast<QDockWidget*>(parentWidget()))
	{
		bool current=dw->isFloating();
		dw->setFloating(!current);
	}
}

void DashboardDockTitleWidget::on_closeTb__clicked(bool)
{
	if(QDockWidget *dw = qobject_cast<QDockWidget*>(parentWidget()))
	{
		dw->close();
	}
}

void DashboardDockTitleWidget::slotUpdateTitle(QString txt)
{
	titleLabel_->setText(txt);
}

//============================================================
//
// DashBoardDock
//
//============================================================

DashboardDock::DashboardDock(DashboardWidget *dw,QWidget * parent) :
	QDockWidget(parent)
{
	DashboardDockTitleWidget *dt=new DashboardDockTitleWidget(this);

	setTitleBarWidget(dt);

	connect(dw,SIGNAL(titleUpdated(QString)),
			dt,SLOT(slotUpdateTitle(QString)));

	dw->populateTitleBar(dt);

	setWidget(dw);
}

void DashboardDock::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
}

void DashboardDock::closeEvent (QCloseEvent *event)
{
	QWidget::closeEvent(event);
	Q_EMIT closeRequested();
}
