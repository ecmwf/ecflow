//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "MessageLabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QProgressBar>
#include <QVariant>
#include <QVBoxLayout>

#include "IconProvider.hpp"
#include "UiLog.hpp"

#include <map>
#include <assert.h>

class MessageLabelData {
public:
	MessageLabelData(QString iconPath,QString title,QColor bg, QColor bgLight,QColor border) :
        title_(title), bg_(bg.name()), border_(border.name())
	{
        int id=IconProvider::add(iconPath,iconPath);
        pix_=IconProvider::pixmap(id,16);
        pixSmall_=IconProvider::pixmap(id,12);

        if(bg == bgLight)
        {
            bg_=bg.name();
        }
        else
        {
            bg_="qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + bg.name() +", stop: 1 " + bgLight.name() + ")";
        }
    }

    MessageLabelData()  {}

	QPixmap pix_;
    QPixmap pixSmall_;
	QString title_;
	QString bg_;
	QString border_;

};

static std::map<MessageLabel::Type,MessageLabelData> typeData;

MessageLabel::MessageLabel(QWidget *parent) :
	QWidget(parent),
    showTypeTitle_(true),
    narrowMode_(false),
    currentType_(NoType)
{
	setProperty("base","1");

	if(typeData.empty())
	{        
        QColor bg(239,244,249);
        QColor bgLight=bg; //bg.lighter(105);
        typeData[InfoType]=MessageLabelData(":/viewer/info.svg","Info",bg,bgLight,QColor(95,145,200));

		bg=QColor(234,215,150);
        bgLight=bg;//bg.lighter(112);
        typeData[WarningType]=MessageLabelData(":/viewer/warning.svg","Warning",bg,bgLight,QColor(226,170,91)); //QColor(226,195,110)); //226,170,91

		bg=QColor(255,231,231);
        bgLight=bg;//bg.lighter(105);
		typeData[ErrorType]=MessageLabelData(":/viewer/error.svg","Error",bg,bgLight,QColor(223,152,152));

        bg=QColor(232,249,236);
        bgLight=bg;//bg.lighter(105);
        typeData[TipType]=MessageLabelData(":/viewer/tip.svg","Tip",bg,bgLight,QColor(190,220,190));
	}

	pixLabel_=new QLabel(this);
	pixLabel_->setAlignment(Qt::AlignCenter);

	msgLabel_=new QLabel(this);
	msgLabel_->setWordWrap(true);
	msgLabel_->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

	loadLabel_=new QLabel(this);

    //progress widget
    progLabel_=new QLabel(this);
    progBar_=new QProgressBar(this);
    progWidget_=new QWidget(this);
    QHBoxLayout* progLayout=new QHBoxLayout(progWidget_);
    progLayout->setContentsMargins(2,2,2,2);
    progLayout->addWidget(progBar_);
    progLayout->addWidget(progLabel_);

    QFont f;
    f.setPointSize(f.pointSize()-1);
    progBar_->setFont(f);
    QFontMetrics fm(f);
    progBar_->setFixedHeight(fm.height()+4);

	layout_=new QHBoxLayout(this);
	layout_->setContentsMargins(2,2,2,2);	    

    QVBoxLayout *loadLayout=new QVBoxLayout();
    loadLayout->addWidget(loadLabel_);
    loadLayout->addStretch(1);
    layout_->addLayout(loadLayout);

    QVBoxLayout *pixLayout=new QVBoxLayout();
    pixLayout->addWidget(pixLabel_);
    pixLayout->addStretch(1);
    layout_->addLayout(pixLayout);

    QVBoxLayout* rightVb=new QVBoxLayout;
    rightVb->addWidget(msgLabel_);
    rightVb->addWidget(progWidget_);
    rightVb->addStretch(1);
    layout_->addLayout(rightVb,1);

    //layout_->addWidget(msgLabel_,1);

	stopLoadLabel();
    stopProgress();

	hide();
}

void MessageLabel::clear()
{
	msgLabel_->setText("");
    message_.clear();
    stopLoadLabel();
    stopProgress();
}

void MessageLabel::showInfo(QString msg)
{
	showMessage(InfoType,msg);
}

void MessageLabel::showWarning(QString msg)
{
	showMessage(WarningType,msg);
}

void MessageLabel::showError(QString msg)
{
	showMessage(ErrorType,msg);
}

void MessageLabel::showTip(QString msg)
{
    showMessage(TipType,msg);
}

void MessageLabel::appendInfo(QString msg)
{
    appendMessage(InfoType,msg);
}

void MessageLabel::appendWarning(QString msg)
{
    appendMessage(WarningType,msg);
}

void MessageLabel::appendError(QString msg)
{
    appendMessage(ErrorType,msg);
}

void MessageLabel::appendTip(QString msg)
{
    appendMessage(TipType,msg);
}

void MessageLabel::showMessage(const Type& type,QString msg)
{
    message_=msg;
    std::map<Type,MessageLabelData>::const_iterator it=typeData.find(type);
	assert(it != typeData.end());

	if(type != currentType_)
	{
		QString sh="QWidget[base=\"1\"] { \
				    background: " + it->second.bg_ + "; \
				    border: 1px solid " +  it->second.border_ + "; \
				    border-radius: 0px;}";

		setStyleSheet(sh);

        pixLabel_->setPixmap(((!narrowMode_)?it->second.pix_:it->second.pixSmall_));

		currentType_=type;
        message_.clear();
	}

    message_=msg;

    QString s=message_;
	s.replace("\n","<br>");
	if(showTypeTitle_)
        s="<b>" + it->second.title_ + ": </b>" + s;

	msgLabel_->setText(s);

	show();
}

void MessageLabel::appendMessage(const Type& type,QString msg)
{
    message_+=msg;
    showMessage(type,message_);
}

void MessageLabel::startLoadLabel()
{
	if(!loadLabel_->movie())
	{
		QMovie *movie = new QMovie(":viewer/spinning_wheel.gif", QByteArray(), loadLabel_);
		loadLabel_->setMovie(movie);
	}
	loadLabel_->show();
	loadLabel_->movie()->start();
}


void MessageLabel::stopLoadLabel()
{
	if(loadLabel_->movie())
	{
		loadLabel_->movie()->stop();
	}

	loadLabel_->hide();
}

void MessageLabel::startProgress(int max)
{
    Q_ASSERT(max >=0 && max <=100);
    progBar_->setRange(0,max);
    progWidget_->show();
}

void MessageLabel::stopProgress()
{
    progWidget_->hide();
    progLabel_->setText("");
    progBar_->setRange(0,0);
}

void MessageLabel::progress(QString text,int value)
{
    Q_ASSERT(value >=0 && value <=100);

    if(progBar_->maximum() == 0)
        progBar_->setMaximum(100);

    progBar_->setValue(value);
    progLabel_->setText(text);

    //UiLog().dbg() << "MessageLabel::progress --> " << value << "%" << " " << text;
}

void MessageLabel::setShowTypeTitle(bool b)
{
	if(showTypeTitle_ != b)
	{
		showTypeTitle_=b;
	}
}

void MessageLabel::setNarrowMode(bool b)
{
    if(b==narrowMode_)
        return;

    narrowMode_=b;

    /*if(!narrowMode_)
    {
        layout_->setContentsMargins(2,2,2,2);
    }
    else
    {
        layout_->setContentsMargins(2,0,2,0);
    }*/
}

