// Copyright 2014 ECMWF.

#ifndef MESSAGELABEL_HPP_
#define MESSAGELABEL_HPP_

#include <QWidget>

class QHBoxLayout;
class QLabel;

class MessageLabel : public QWidget
{
public:
	explicit MessageLabel(QWidget *parent=0);

	enum Type {NoType,InfoType,WarningType,ErrorType};

	void showInfo(QString);
	void showWarning(QString);
	void showError(QString);
	void startLoadLabel();
	void stopLoadLabel();
	void setShowTypeTitle(bool);
	void clear();
	void useNarrowMode(bool);;

private:
	void showMessage(const Type&,QString);

	bool showTypeTitle_;
	Type currentType_;
	QLabel *pixLabel_;
	QLabel* msgLabel_;
	QLabel* loadLabel_;
	QHBoxLayout* layout_;

};

#endif

