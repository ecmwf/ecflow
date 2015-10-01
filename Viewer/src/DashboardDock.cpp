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

#include <QLinearGradient>
#include <QPalette>

#include "DashboardWidget.hpp"

DashboardDockTitleWidget::DashboardDockTitleWidget(QWidget *parent) :
		QWidget(parent)
{
	setupUi(this);

	setProperty("dockTitle","1");

	//We define the style here because it did not work from the qss file
	QPalette p=palette();
	QLinearGradient gr(0,0,0,1);
	gr.setCoordinateMode(QGradient::ObjectBoundingMode);
	gr.setColorAt(0,QColor(130,130,130));
	gr.setColorAt(0.4,QColor(120,120,120));
	gr.setColorAt(0.41,QColor(112,112,112));
	gr.setColorAt(1,QColor(95,95,95));
	p.setBrush(QPalette::Window,gr);
	setPalette(p);

	p=titleLabel_->palette();
	p.setColor(QPalette::WindowText,Qt::white);
	titleLabel_->setPalette(p);

    optionsTb_->setProperty("docktitle","1");
    closeTb_->setProperty("docktitle","1");

	//Set the initial state of the float tool button
	if(QDockWidget *dw = qobject_cast<QDockWidget*>(parentWidget()))
	{
		if(!dw->features().testFlag(QDockWidget::DockWidgetFloatable))
		{
            //floatTb_->setEnabled(false);
            floatTb_->hide();
		}
	}
}

void DashboardDockTitleWidget::addActions(QList<QAction*> lst)
{  
    Q_FOREACH(QAction* ac,lst)
	{
    	 QToolButton *tb=new QToolButton(this);
         tb->setProperty("docktitle","1");
         tb->setDefaultAction(ac);
         tb->setAutoRaise(true);
         tb->setPopupMode(QToolButton::InstantPopup);
         //tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
         actionLayout_->addWidget(tb);

         if(!ac->isEnabled())
             tb->hide();

         actionTbList_ << tb;

         connect(ac,SIGNAL(changed()),
                 this,SLOT(slotActionChanged()));
    }

    //Hide the separator line if no buttons were added
    if(actionTbList_.isEmpty())
        line_->hide();
}

void DashboardDockTitleWidget::slotActionChanged()
{
    Q_FOREACH(QToolButton *tb, actionTbList_)
    {
        if(!tb->isEnabled())
        {
            tb->hide();
        }
        else
        {
            tb->show();
        }
    }
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
		if(dw->features().testFlag(QDockWidget::DockWidgetFloatable))
		{
			bool current=dw->isFloating();
			dw->setFloating(!current);
		}
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
	setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetMovable);

	DashboardDockTitleWidget *dt=new DashboardDockTitleWidget(this);

	setTitleBarWidget(dt);

	dt->addActions(dw->dockTitleActions());

	connect(dw,SIGNAL(titleUpdated(QString)),
			dt,SLOT(slotUpdateTitle(QString)));

    dw->populateDockTitleBar(dt);

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
