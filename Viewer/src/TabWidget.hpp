//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TABWIDGET_HPP_
#define TABWIDGET_HPP_

#include <QWidget>

class QMenu;
class QStackedWidget;
class QTabBar;
class QToolButton;
class QVBoxLayout;

class TabWidget : public QWidget
{
    Q_OBJECT

public:
    TabWidget(QWidget *parent=0);

	int currentIndex() const;
	int indexOfWidget(QWidget*) const;
	QWidget *widget(int) const;
	QWidget *currentWidget() const;
	void checkTabStatus();
	void addTab(QWidget *,QPixmap,QString);
	void setTabText(int,QString);
	void setTabIcon(int,QPixmap);
	int count() const;
	void clear();

public Q_SLOTS:
	void removeTab(int);
	void removeOtherTabs(int);
	void setCurrentIndex(int);

private Q_SLOTS:
	void slotContextMenu(const QPoint&);
	void currentTabChanged(int index);
	void tabMoved(int from,int to);

Q_SIGNALS:
    void currentIndexChanged(int);
	void newTabRequested();

protected:
	//virtual MvQContextItemSet* cmSet()=0;
	virtual void tabBarCommand(QString,int)=0;

private:
	QTabBar *bar_;
	QStackedWidget *stacked_;
	QToolButton* addTb_;
};

#endif
