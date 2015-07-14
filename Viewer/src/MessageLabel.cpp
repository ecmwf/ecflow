// Copyright 2014 ECMWF.

#include "MessageLabel.hpp"

#include <map>
#include <assert.h>

class MessageLabelData {
public:
	MessageLabelData(QString icon,QString title,QString bg, QString border) :
		icon_(icon), title_(title), bg_(bg), border_(border) {}

	MessageLabelData()  {};

	QString icon_;
	QString title_;
	QString bg_;
	QString border_;
};

static std::map<MessageLabel::Type,MessageLabelData> typeData;


MessageLabel::MessageLabel(QWidget *parent) :
	QLabel(parent),
	currentType_(NoType)
{
	if(typeData.empty())
	{
		typeData[InfoType]=MessageLabelData(":/viewer/info.svg","Info","rgb(227,242,252)","rgb(180,194,230)");
		typeData[WarningType]=MessageLabelData(":/viewer/warning.svg","Warning","rgb(255,198,63)","rgb(255,140,0)");
		typeData[ErrorType]=MessageLabelData(":/viewer/error.svg","Error","rgb(255,231,231)","rgb(223,152,152)");
	}

	//setObjectName("messageLabel");
	setAutoFillBackground(true);
	setWordWrap(true);
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
		QString sh="QLabel{ \
		    background: " + it->second.bg_ + "; \
		    border: 1px solid " +  it->second.border_ + "; \
		    border-radius: 4px;}";

		setStyleSheet(sh);

		currentType_=type;
	}

	msg.replace("\n","<br>");

	QString s="<table><tr><td><img src=\'" + it->second.icon_ + "\' /></td><td><b>" + it->second.title_ + ": </b>";
	s+="<br>" + msg + "</td></tr></table>";
	show();
	setText(s);
}
