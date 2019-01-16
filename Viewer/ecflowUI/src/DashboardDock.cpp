//============================================================================
// Copyright 2009-2019 ECMWF.
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
#include "IconProvider.hpp"

DashboardDockTitleWidget::DashboardDockTitleWidget(QWidget *parent) :
        QWidget(parent),
        titleBc_(0)
{
	setupUi(this);

	setProperty("dockTitle","1");

	//We define the style here because it did not work from the qss file
	QPalette p=palette();
	QLinearGradient gr(0,0,0,1);
	gr.setCoordinateMode(QGradient::ObjectBoundingMode);
    gr.setColorAt(0,QColor(126,126,126));
    gr.setColorAt(0.4,QColor(105,105,105));
    gr.setColorAt(0.41,QColor(97,97,97));
    gr.setColorAt(1,QColor(70,70,70));
	p.setBrush(QPalette::Window,gr);
	setPalette(p);

    //Add warning icon
    IconProvider::add(":viewer/warning.svg","warning");

    //Title icon + text
	p=titleLabel_->palette();
	p.setColor(QPalette::WindowText,Qt::white);
	titleLabel_->setPalette(p);
    titleLabel_->hide();
    titleIconLabel_->hide();

    detachedTb_->setProperty("docktitle","1");
    maxTb_->setProperty("docktitle","1");
    optionsTb_->setProperty("docktitle","1");
    closeTb_->setProperty("docktitle","1");
}

void DashboardDockTitleWidget::setBcWidget(QWidget *w)
{
    Q_ASSERT(mainLayout->count() > 1);
    Q_ASSERT(w);
    titleBc_=w;
    mainLayout->insertWidget(1,titleBc_,1);
#if 0
    for(int i=0; i < mainLayout->count(); i++)
    {
        if(QLayoutItem* item=mainLayout->itemAt(i))
        {
            if(item->widget() == titleIconLabel_)
            {
                titleBc_=w;
                mainLayout->insertWidget(i,titleBc_,1);
                return;
            }
        }
    }
#endif

    Q_ASSERT(titleBc_);
}

void DashboardDockTitleWidget::setDetachedAction(QAction *ac)
{
    detachedTb_->setDefaultAction(ac);
}

void DashboardDockTitleWidget::setMaximisedAction(QAction *ac)
{
    maxTb_->setDefaultAction(ac);
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

         QPalette p=tb->palette();
         p.setColor(QPalette::ButtonText,Qt::white);
         tb->setPalette(p);

         actionLayout_->addWidget(tb);

         if(!ac->isEnabled())
             tb->hide();

         actionTbList_ << tb;

         connect(ac,SIGNAL(changed()),
                 this,SLOT(slotActionChanged()));
    }

    //Hide the separator line if no buttons were added
    //if(actionTbList_.isEmpty())
    //    line_->hide();
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

#if 0
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
#endif

void DashboardDockTitleWidget::on_closeTb__clicked(bool)
{
	if(QDockWidget *dw = qobject_cast<QDockWidget*>(parentWidget()))
	{
		dw->close();
	}
}

void DashboardDockTitleWidget::slotUpdateTitle(QString txt,QString type)
{
    if(txt.isEmpty())
    {
       titleLabel_->hide();
       titleIconLabel_->hide();
    }
    else
    {
        if(type == "warning")
        {
            titleIconLabel_->setPixmap(IconProvider::pixmap("warning",16));
            titleIconLabel_->show();
            txt.prepend(" ");
            txt.append("  ");
        }
        else
        {
            titleIconLabel_->show();
        }

        titleLabel_->show();
        titleLabel_->setText(txt);
    }
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

    dt->setDetachedAction(dw->detachedAction());
    dt->setMaximisedAction(dw->maximisedAction());
	dt->addActions(dw->dockTitleActions());

    connect(dw,SIGNAL(titleUpdated(QString,QString)),
            dt,SLOT(slotUpdateTitle(QString,QString)));

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
