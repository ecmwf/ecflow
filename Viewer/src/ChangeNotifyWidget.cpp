//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ChangeNotifyWidget.hpp"

#include <QHBoxLayout>
#include <QPainter>
#include <QSignalMapper>
#include <QToolButton>

#include "ChangeNotify.hpp"
#include "VNodeList.hpp"
#include "VProperty.hpp"

#include <vector>

std::vector<ChangeNotifyWidget*> ChangeNotifyWidget::widgets_;

ChangeNotifyButton::ChangeNotifyButton(QWidget* parent) :
	QToolButton(parent),
	notifier_(0)
{
	setProperty("notify","1");
	setAutoRaise(true);
	setIconSize(QSize(20,20));
}

void ChangeNotifyButton::setNotifier(ChangeNotify* notifier)
{
	notifier_=notifier;

	if(notifier_->prop())
		setToolTip(notifier_->prop()->param("tooltip"));

	connect(this,SIGNAL(clicked(bool)),
			this,SLOT(slotClicked(bool)));

	connect(notifier_->data(),SIGNAL(endAppendRow()),
			this,SLOT(slotAppend()));

	connect(notifier_->data(),SIGNAL(endReset()),
				this,SLOT(slotReset()));

	updateIcon();
}

void ChangeNotifyButton::slotAppend()
{
	updateIcon();
}

void ChangeNotifyButton::slotReset()
{
	updateIcon();
}

void ChangeNotifyButton::slotClicked(bool)
{
	ChangeNotify::showDialog(notifier_->id());
}

void ChangeNotifyButton::updateIcon()
{
	QString text;
	QString numText;

	if(notifier_->prop())
	{
		text=notifier_->prop()->param("widgetText");
	}

	if(notifier_->data())
	{
		int num=notifier_->data()->size();
		if(num > 0 && num < 10)
			numText=QString::number(num);
		else if(num > 10)
			numText="9+";

	}

	QColor bg;
	QColor fg;
	QColor border;

	if(notifier_->prop())
	{
		bg=notifier_->prop()->paramToColour("background");
		fg=notifier_->prop()->paramToColour("foreground");
		border=notifier_->prop()->paramToColour("border");
	}

	QFont f;
	f.setBold(true);
	f.setPointSize(f.pointSize()+1);
	QFontMetrics fm(f);
	int w;
	if(!numText.isEmpty())
		w=fm.width(text) + 6 + fm.width(numText) + 2;
	else
		w=fm.width(text) + 6;

	int h=fm.height()+6;

	QPixmap pix(w,h);
	pix.fill(QColor(255,255,255,0));
	QPainter painter(&pix);
	painter.setRenderHint(QPainter::Antialiasing,true);
	painter.setRenderHint(QPainter::TextAntialiasing,true);

	QRect textRect(0,0,fm.width(text)+6,h);
	painter.setBrush(bg);
	painter.setPen(border);
	painter.drawRoundedRect(textRect,2,2);
	painter.setPen(fg);
	painter.setFont(f);
	painter.drawText(textRect,Qt::AlignHCenter|Qt::AlignVCenter,text);

	if(!numText.isEmpty())
	{
		QRect numRect(textRect.right()-1,0,fm.width(numText)+4,fm.ascent()+4);
		painter.setBrush(QColor(58,126,194));
		painter.setPen(Qt::white);
		painter.drawRoundedRect(numRect,4,4);
		painter.setFont(f);
		painter.drawText(numRect,Qt::AlignHCenter|Qt::AlignVCenter,numText);
	}

	setIconSize(QSize(w,h));
	setIcon(pix);

}


ChangeNotifyWidget::ChangeNotifyWidget(QWidget *parent) : QWidget(parent)
{
	layout_=new QHBoxLayout(this);
	layout_->setContentsMargins(0,0,0,0);
	layout_->setSpacing(0);

	ChangeNotify::populate(this);

	widgets_.push_back(this);
}

ChangeNotifyWidget::~ChangeNotifyWidget()
{
	std::vector<ChangeNotifyWidget*>::iterator it=std::find(widgets_.begin(),widgets_.end(),this);
	if(it != widgets_.end())
		widgets_.erase(it);
}

ChangeNotifyButton* ChangeNotifyWidget::findButton(const std::string& id)
{
	std::map<std::string,ChangeNotifyButton*>::const_iterator it=buttons_.find(id);
	if(it != buttons_.end())
		return it->second;

	return 0;
}


void ChangeNotifyWidget::addTb(ChangeNotify* notifier)
{
	ChangeNotifyButton *tb=new ChangeNotifyButton(this);
	tb->setNotifier(notifier);
	layout_->addWidget(tb);
	if(!notifier->isEnabled())
		tb->setEnabled(false);

	buttons_[notifier->id()]=tb;
}

void ChangeNotifyWidget::setEnabled(const std::string& id,bool b)
{
	for(std::vector<ChangeNotifyWidget*>::iterator it=widgets_.begin(); it!= widgets_.end(); it++)
	{
		if(ChangeNotifyButton* tb=(*it)->findButton(id))
		{
			tb->setEnabled(b);
		}
	}
}

/*
void ChangeNotifyWidget::update(const std::string& id)
{
	std::map<std::string,QToolButton*>::iterator it=tbMap_.find(id)
	if(it != tbMap_.end())
	{
		QToolButton *tb=it->second;

		ChangeNotify* n=ChangeNotify::find(id)

		if(it != widgets_.end())
}
*/
/*
void ChangeNotifyWidget::changed(const std::string& id)
{
	for(std::vector<ChangeNotifyWidget*>::iterator it=widgets_.begin(); it != widgets_.end(); it++)
	{
		(*it)->update(id);
	}

	//ids <<  id;
}
*/
