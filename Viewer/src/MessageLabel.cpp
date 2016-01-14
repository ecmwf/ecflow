// Copyright 2014 ECMWF.

#include "MessageLabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QVariant>

#include "IconProvider.hpp"

#include <map>
#include <assert.h>

class MessageLabelData {
public:
	MessageLabelData(QString iconPath,QString title,QColor bg, QColor bgLight,QColor border) :
	title_(title), bg_(bg.name()), border_(border.name())
	{
		int id=IconProvider::add(iconPath,iconPath);
		pix_=IconProvider::pixmap(id,18);

		bg_="qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + bg.name() +", stop: 1 " + bgLight.name() + ")";
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
	currentType_(NoType),
	showTypeTitle_(true)
{
	setProperty("base","1");

	if(typeData.empty())
	{
		QColor bg(236,246,252);
		QColor bgLight=bg.lighter(105);
		typeData[InfoType]=MessageLabelData(":/viewer/info.svg","Info",bg,bgLight,QColor(180,194,230));

		bg=QColor(234,215,150);
		bgLight=bg.lighter(112);
		typeData[WarningType]=MessageLabelData(":/viewer/warning.svg","Warning",bg,bgLight,QColor(226,195,110)); //255,198,63

		bg=QColor(255,231,231);
		bgLight=bg.lighter(105);
		typeData[ErrorType]=MessageLabelData(":/viewer/error.svg","Error",bg,bgLight,QColor(223,152,152));
	}

	pixLabel_=new QLabel(this);
	pixLabel_->setAlignment(Qt::AlignCenter);

	msgLabel_=new QLabel(this);
	msgLabel_->setWordWrap(true);
	msgLabel_->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

	loadLabel_=new QLabel(this);

	layout_=new QHBoxLayout(this);
	layout_->setContentsMargins(2,2,2,2);
	layout_->addWidget(loadLabel_);
	layout_->addWidget(pixLabel_);
	layout_->addWidget(msgLabel_,1);

	stopLoadLabel();

	hide();
}

void MessageLabel::clear()
{
	msgLabel_->setText("");
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
	if(showTypeTitle_)
		s="<b>" + it->second.title_ + ": </b><br>" + s;

	msgLabel_->setText(s);

	show();
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

void MessageLabel::setShowTypeTitle(bool b)
{
	if(showTypeTitle_ != b)
	{
		showTypeTitle_=b;
	}
}

void MessageLabel::useNarrowMode(bool b)
{
	if(!b)
		layout_->setContentsMargins(2,2,2,2);
	else
		layout_->setContentsMargins(2,0,2,0);
}

