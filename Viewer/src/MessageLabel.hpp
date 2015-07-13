// Copyright 2014 ECMWF.

#ifndef MESSAGELABEL_HPP_
#define MESSAGELABEL_HPP_

#include <QLabel>

class MessageLabel : public QLabel
{
public:
	explicit MessageLabel(QWidget *parent=0);

	enum Type {NoType,InfoType,WarningType,ErrorType};

	void showInfo(QString);
	void showWarning(QString);
	void showError(QString);

private:
	void showMessage(const Type&,QString);

	Type currentType_;

};

#endif

