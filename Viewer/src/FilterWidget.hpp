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
#include "ServerFilter.hpp"
#include "ServerList.hpp"

class QToolButton;
class VParam;
class VParamSet;
class ServerFilter;


class VParamFilterMenu : public QObject
{
Q_OBJECT

public:
	enum DecorMode {NoDecor,ColourDecor,PixmapDecor};

	VParamFilterMenu(QMenu* parent,VParamSet* filter,DecorMode decorMode=NoDecor);
	void reload();

protected Q_SLOTS:
	void slotChanged(bool);
	void slotSelectAll(bool);
	void slotUnselectAll(bool);

protected:
	void addAction(QString name,QString id);

	QMenu*  menu_;
	VParamSet* filter_;
	DecorMode decorMode_;
};

/*
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
*/

class ServerFilterMenu : public QObject, public ServerListObserver, public ServerFilterObserver
{
Q_OBJECT

public:
	explicit ServerFilterMenu(QMenu* parent);
	~ServerFilterMenu();

	void reload(ServerFilter*);

	//From ServerListObserver
	void notifyServerListChanged();

	//From ConfigObserver
	void notifyServerFilterAdded(ServerItem*);
	void notifyServerFilterRemoved(ServerItem*);
	void notifyServerFilterChanged(ServerItem*);

protected Q_SLOTS:
	void slotChanged(bool);

protected:
	void init();
	void clear();
	void addAction(QString name,int id);
	void reload();

	QMenu*  menu_;
	QList<QAction*> acLst_;
	ServerFilter* filter_;
};




class FilterWidget : public QWidget
{
Q_OBJECT

public:
	explicit FilterWidget(QWidget* parent=0);
	void reload(VParamSet*);

protected Q_SLOTS:
	void slotChanged(bool);

Q_SIGNALS:
	void filterChanged(QSet<DState::State>);

private:
	QToolButton* createButton(QString,QString,QColor);

	QMap<DState::State,QToolButton*> items_;
	VParamSet* data_;
};

#endif
