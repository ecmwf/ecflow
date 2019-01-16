//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LineEdit.hpp"

#include <QLabel>
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>

LineEdit::LineEdit(QWidget *parent):
      QLineEdit(parent),
      iconLabel_(0)
{
	clearTb_=new QToolButton(this);
	QPixmap pix(":/viewer/clear_left.svg");
    clearTb_->setIcon(pix);
    clearTb_->setIconSize(QSize(12,12));
  	clearTb_->setAutoRaise(true);
	clearTb_->setCursor(Qt::ArrowCursor);
	clearTb_->setToolTip(tr("Clear text"));
	clearTb_->setObjectName("clearTextTb");

    clearTb_->setStyleSheet("QToolButton{ border: node; padding: 0px;}");
    
	connect(clearTb_,SIGNAL(clicked()),
		this,SLOT(slotClear()));

	adjustSize();

	/*QSize tbSize=clearTb_->sizeHint();

   	int frame = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(tbSize.width()+frame+1));
	QSize minSize = minimumSizeHint();
	setMinimumSize(qMax(minSize.width(),tbSize.height()+frame*2+2),
                       qMax(minSize.height(),tbSize.height()+frame*2+2));*/

}

void LineEdit::setDecoration(QPixmap pix)
{
 	if(!iconLabel_)
  		iconLabel_=new QLabel(this);

	iconLabel_->setPixmap(pix);
	iconLabel_->setProperty("lineEdit","true");

	adjustSize();

	/*QSize icSize=iconLabel_->sizeHint();
	QSize tbSize=clearTb_->sizeHint();

   	int frame = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-left: %1px; } ").arg(icSize.width()+frame+1));
	QSize minSize = minimumSizeHint();
	setMinimumSize(qMax(minSize.width(),icSize.width()+tbSize.width()+frame*2+2),
                       qMax(minSize.height(),tbSize.height()+frame*2+2));*/
}


void LineEdit::adjustSize()
{
   	int frame = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	QSize tbSize=clearTb_->sizeHint();
	setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(tbSize.width()+frame+1));

	int leftWidth=0;
	if(iconLabel_)
	{
		QSize icSize=iconLabel_->sizeHint();
	  	setStyleSheet(QString("QLineEdit { padding-left: %1px; } ").arg(icSize.width()+frame+1));
		leftWidth=+icSize.width();
	}
	QSize minSize = minimumSizeHint();
	setMinimumSize(qMax(minSize.width(),leftWidth+tbSize.width()+frame*2+2),
                       qMax(minSize.height(),tbSize.height()+frame*2+2));
}


void LineEdit::resizeEvent(QResizeEvent *)
{
	int frame = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

	QSize tbSize = clearTb_->sizeHint();
	clearTb_->move(rect().right()-frame-tbSize.width(),
		      (rect().bottom()+ 1-tbSize.height())/2);

	if(iconLabel_)
	{
	  	QSize icSize=iconLabel_->sizeHint();
		iconLabel_->move(rect().left()+frame+1,
		      (rect().bottom()+ 1-icSize.height())/2);
	}
}


void LineEdit::slotClear()
{
  	clear();
  	Q_EMIT textCleared();
}

