// Copyright 2014 ECMWF.

#include "MessageLabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QVariant>

#include "IconProvider.hpp"

#include <map>
#include <assert.h>

class MessageLabelData {
public:
	MessageLabelData(QString iconPath,QString title,QString bg, QString border) :
	title_(title), bg_(bg), border_(border)
	{
		int id=IconProvider::add(iconPath,iconPath);
		pix_=IconProvider::pixmap(id,20);
	}

	MessageLabelData()  {};

	QPixmap pix_;
	QString title_;
	QString bg_;
	QString border_;
};

static std::map<MessageLabel::Type,MessageLabelData> typeData;

MessageLabel::MessageLabel(QWidget *parent) :
	QWidget(parent),
	currentType_(NoType)
{
	setProperty("base","1");

	if(typeData.empty())
	{
		typeData[InfoType]=MessageLabelData(":/viewer/info.svg","Info","rgb(236,246,252)","rgb(180,194,230)");
		typeData[WarningType]=MessageLabelData(":/viewer/warning.svg","Warning","rgb(255,198,63)","rgb(255,140,0)");
		typeData[ErrorType]=MessageLabelData(":/viewer/error.svg","Error","rgb(255,231,231)","rgb(223,152,152)");
	}

	pixLabel_=new QLabel(this);
	pixLabel_->setAlignment(Qt::AlignCenter);

	msgLabel_=new QLabel(this);
	msgLabel_->setWordWrap(true);

	QHBoxLayout* hb=new QHBoxLayout(this);
	hb->setContentsMargins(2,2,2,2);
	hb->addWidget(pixLabel_);
	hb->addWidget(msgLabel_,1);

	hide();
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

void MessageLabel::showMessage(const Type& type,QString msg)
{
	std::map<Type,MessageLabelData>::const_iterator it=typeData.find(type);
	assert(it != typeData.end());

	if(type != currentType_)
	{
		QString sh="QWidget[base=\"1\"] { \
				    background: " + it->second.bg_ + "; \
				    border: 1px solid " +  it->second.border_ + "; \
				    border-radius: 0px;}";

		setStyleSheet(sh);

		pixLabel_->setPixmap(it->second.pix_);

		currentType_=type;
	}

	QString s=msg;
	s.replace("\n","<br>");
	s="<b>" + it->second.title_ + ": </b><br>" + s;
	msgLabel_->setText(s);

	show();
}
