//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_CHANGENOTIFYWIDGET_HPP_
#define VIEWER_SRC_CHANGENOTIFYWIDGET_HPP_

#include <QToolButton>
#include <QWidget>

#include <map>
#include <string>
#include <vector>

class QHBoxLayout;
class QSignalMapper;

class ChangeNotify;
class VProperty;

class ChangeNotifyButton : public QToolButton
{
Q_OBJECT

public:
	explicit ChangeNotifyButton(QWidget* parent=0);

	void setNotifier(ChangeNotify*);

public Q_SLOTS:
	void slotAppend();
	void slotReset();
	void slotClicked(bool);

protected:
	ChangeNotify* notifier_;
};

class ChangeNotifyWidget : public QWidget
{
friend class ChangeNotify;

public:
	explicit ChangeNotifyWidget(QWidget *parent=0);
	~ChangeNotifyWidget();

	//static void changed(QString id);
	//static void add(const std::string& id,VProperty* prop);

protected:
	void addTb(ChangeNotify*);

	QHBoxLayout* layout_;
	static std::vector<ChangeNotifyWidget*> widgets_;
};

#endif /* VIEWER_SRC_CHANGENOTIFYWIDGET_HPP_ */
