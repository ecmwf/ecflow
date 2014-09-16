//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef FILTERWIDGET_HPP_
#define FILTERWIDGET_HPP_

#include <QMap>
#include <QMenu>
#include <QSet>
#include <QWidget>

#include "DState.hpp"
#include "ServerList.hpp"

class QToolButton;
class VParam;
class VFilter;
class ServerFilter;


class AbstractFilterMenu : public QObject
{
Q_OBJECT

public:
	AbstractFilterMenu(QMenu* parent,const std::vector<VParam*>&);
	void reload(VFilter*);

protected slots:
	void slotChanged(bool);

protected:
	void addAction(QString name,int id);

	QMenu*  menu_;
	VFilter* filter_;
};

class StateFilterMenu : public AbstractFilterMenu
{
public:
	StateFilterMenu(QMenu* parent);
};

class AttributeFilterMenu : public AbstractFilterMenu
{
public:
	AttributeFilterMenu(QMenu* parent);
};

class IconFilterMenu : public AbstractFilterMenu
{
public:
	IconFilterMenu(QMenu* parent);
};


class ServerFilterMenu : public QObject, public ServerListObserver
{
Q_OBJECT

public:
	ServerFilterMenu(QMenu* parent);
	~ServerFilterMenu();

	void reloadFilter(ServerFilter*);

	//From ServerListObserver
	void notifyServerListChanged();

protected slots:
	void slotChanged(bool);

protected:
	void init();
	void clear();
	void addAction(QString name,int id);

	QMenu*  menu_;
	QList<QAction*> acLst_;
	ServerFilter* filter_;
};




class FilterWidget : public QWidget
{
Q_OBJECT

public:
	FilterWidget(QWidget* parent=0);
	void reload(VFilter*);

protected slots:
	void slotChanged(bool);

signals:
	void filterChanged(QSet<DState::State>);

private:
	QToolButton* createButton(QString,QString,QColor);

	QMap<DState::State,QToolButton*> items_;
	VFilter* data_;
};

#endif
