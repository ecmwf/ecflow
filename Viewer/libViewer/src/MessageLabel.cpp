//============================================================================
// Copyright 2009- ECMWF.
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
#include <QPainter>
#include <QProgressBar>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

#include "IconProvider.hpp"
#include "UiLog.hpp"

#include <cassert>
#include <map>

//===================================================
//
// MessageLabelLoadWidget
//
//===================================================

MessageLabelLoadWidget::MessageLabelLoadWidget(QWidget *parent) : QWidget(parent)
{
    auto* layout=new QHBoxLayout(this);

    loadTextLabel_=new QLabel("In progress ...",this);
    loadIconLabel_=new QLabel(this);
    loadCancelTb_=new QToolButton(this);
    loadCancelTb_->setText(tr("Cancel"));
    loadCancelTb_->setIcon(QPixmap(":/viewer/remove.svg"));
    loadCancelTb_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    layout->setContentsMargins(2,2,2,2);
    layout->addWidget(loadIconLabel_);
    layout->addWidget(loadTextLabel_);
    layout->addWidget(loadCancelTb_);
    layout->addStretch(1);

    connect(loadCancelTb_,SIGNAL(clicked()),
            this,SLOT(cancelButtonClicked()));
}

void MessageLabelLoadWidget::startLoadLabel(bool showCancelButton)
{
    if(!loadIconLabel_->movie())
    {
        QMovie *movie = new QMovie(":viewer/spinning_wheel.gif", QByteArray(), loadIconLabel_);
        loadIconLabel_->setMovie(movie);
    }

    show();
    loadCancelTb_->setVisible(showCancelButton);
    loadIconLabel_->movie()->start();
}

void MessageLabelLoadWidget::cancelButtonClicked()
{
     stopLoadLabel();
     Q_EMIT stoppedByButton();
}

void MessageLabelLoadWidget::stopLoadLabel()
{
    if(loadIconLabel_->movie())
    {
        loadIconLabel_->movie()->stop();
    }
    loadCancelTb_->hide();
    hide();
}

//===================================================
//
// MessageLabelProgWidget
//
//===================================================

MessageLabelProgWidget::MessageLabelProgWidget(QWidget *parent) : QWidget(parent)
{
    auto* layout=new QHBoxLayout(this);
    layout->setContentsMargins(2,2,2,2);

    progLabel_=new QLabel(this);
    progBar_=new QProgressBar(this);
    //progWidget_=new QWidget(this);
    progCancelTb_=new QToolButton(this);
    progCancelTb_->setText(tr("Cancel"));
    progCancelTb_->hide();

    layout->addWidget(progBar_);
    layout->addWidget(progLabel_);
    layout->addWidget(progCancelTb_);

    QFont f;
    f.setPointSize(f.pointSize()-1);
    progBar_->setFont(f);
    QFontMetrics fm(f);
    progBar_->setFixedHeight(fm.height()+4);

    delayedProgressTimer_ = new QTimer(this);
    delayedProgressTimer_->setSingleShot(true);
    delayedProgressTimer_->setInterval(delayInMs_);

    connect(delayedProgressTimer_,SIGNAL(timeout()),
            this, SLOT(showDelayedProgress()));

    connect(progCancelTb_,SIGNAL(clicked()),
            this,SLOT(cancelButtonClicked()));
}

void MessageLabelProgWidget::startProgress(int max)
{
    Q_ASSERT(max >=0 && max <=100);
    progBar_->setRange(0,max);
    show();
}

void MessageLabelProgWidget::cancelButtonClicked()
{
     stopProgress();
     Q_EMIT stoppedByButton();
}

void MessageLabelProgWidget::stopProgress()
{
    hide();
    progLabel_->setText("");
    progBar_->setRange(0,0);
    delayedProgressTimer_->stop();
    delayedProgressDef_.clear();
}

void MessageLabelProgWidget::progress(QString text,int value)
{
    Q_ASSERT(value >=0 && value <=100);

    if(progBar_->maximum() == 0)
        progBar_->setMaximum(100);

    if (delayedProgressTimer_->isActive()) {
       delayedProgressDef_.progText_ = text;
       delayedProgressDef_.progVal_ = value;
    } else {
        progBar_->setValue(value);
        progLabel_->setText(text);
    }

    //UiLog().dbg() << "MessageLabel::progress --> " << value << "%" << " " << text;
}

void MessageLabelProgWidget::startDelayedProgress(QString infoText, int max)
{
    delayedProgressDef_ = {infoText, max};
    delayedProgressTimer_->stop();
    delayedProgressTimer_->start();
}

void MessageLabelProgWidget::showDelayedProgress()
{
    delayedProgressTimer_->stop();
    Q_EMIT showInfoRequested(delayedProgressDef_.infoText_);
    startProgress(delayedProgressDef_.max_);
    if (!delayedProgressDef_.progText_.isEmpty() || delayedProgressDef_.progVal_ >=  delayedProgressDef_.max_) {
        progress(delayedProgressDef_.progText_, delayedProgressDef_.progVal_);
    }
    delayedProgressDef_.clear();
}

//===================================================
//
// MessageLabel
//
//===================================================

class MessageLabelDefData {
public:
    MessageLabelDefData(QString iconPath,QString title,QColor bg, QColor bgLight,QColor border) :
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

    MessageLabelDefData()  = default;

    QPixmap pix_;
    QPixmap pixSmall_;
    QString title_;
    QString bg_;
    QString border_;

};

static std::map<MessageLabel::Type,MessageLabelDefData> typeData;

MessageLabel::MessageLabel(QWidget *parent) :
    QWidget(parent)
{
	setProperty("base","1");

	if(typeData.empty())
	{        
        QColor bg(239,244,249);
        QColor bgLight=bg; //bg.lighter(105);
        typeData[InfoType]=MessageLabelDefData(":/viewer/info.svg","Info",bg,bgLight,QColor(95,145,200));

		bg=QColor(234,215,150);
        bgLight=bg;//bg.lighter(112);
        typeData[WarningType]=MessageLabelDefData(":/viewer/warning.svg","Warning",bg,bgLight,QColor(226,170,91)); //QColor(226,195,110)); //226,170,91

		bg=QColor(255,231,231);
        bgLight=bg;//bg.lighter(105);
        typeData[ErrorType]=MessageLabelDefData(":/viewer/error.svg","Error",bg,bgLight,QColor(223,152,152));

        bg=QColor(232,249,236);
        bgLight=bg;//bg.lighter(105);
        typeData[TipType]=MessageLabelDefData(":/viewer/tip.svg","Tip",bg,bgLight,QColor(190,220,190));
	}

	pixLabel_=new QLabel(this);
	pixLabel_->setAlignment(Qt::AlignCenter);

	msgLabel_=new QLabel(this);
    msgLabel_->setWordWrap(true);
	msgLabel_->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    //load widget
    loadWidget_=new MessageLabelLoadWidget(this);

    //progress widget
    progWidget_ = new MessageLabelProgWidget(this);

	layout_=new QHBoxLayout(this);
	layout_->setContentsMargins(2,2,2,2);	    

    auto *pixLayout=new QVBoxLayout();
    pixLayout->addWidget(pixLabel_);
    pixLayout->addStretch(1);
    layout_->addLayout(pixLayout);

    auto* rightVb=new QVBoxLayout;
    rightVb->addWidget(msgLabel_);
    rightVb->addWidget(progWidget_);
    rightVb->addWidget(loadWidget_);
    rightVb->addStretch(1);
    layout_->addLayout(rightVb,1);

	stopLoadLabel();
    stopProgress();

	hide();

    connect(loadWidget_, SIGNAL(stoppedByButton()),
            this, SIGNAL(loadStoppedByButton()));

    connect(progWidget_, SIGNAL(showInfoRequested(QString)),
            this, SLOT(showInfo(QString)));

    connect(progWidget_, SIGNAL(stoppedByButton()),
            this, SIGNAL(progressStoppedByButton()));

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
    std::map<Type,MessageLabelDefData>::const_iterator it=typeData.find(type);
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

    if(s.endsWith("<br>"))
        s=s.left(s.size()-4);

    msgLabel_->setText(s);

	show();
}

void MessageLabel::appendMessage(const Type& type,QString msg)
{
    message_+=msg;
    showMessage(type,message_);
}

void MessageLabel::startLoadLabel(bool showCancelButton)
{
    loadWidget_->startLoadLabel(showCancelButton);
}

void MessageLabel::stopLoadLabel()
{
    loadWidget_->stopLoadLabel();
}

void MessageLabel::startProgress(int max)
{
    progWidget_->startProgress(max);
}

void MessageLabel::stopProgress()
{
    progWidget_->stopProgress();
}

void MessageLabel::progress(QString text,int value)
{
    progWidget_->progress(text, value);
}

void MessageLabel::startDelayedProgress(QString infoText, int max)
{
    progWidget_->startDelayedProgress(infoText, max);
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

void  MessageLabel::paintEvent(QPaintEvent *)
{
     QStyleOption opt;
     opt.initFrom(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
