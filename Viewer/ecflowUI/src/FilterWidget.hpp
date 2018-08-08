//============================================================================
// Copyright 2009-2017 ECMWF.
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
	enum ItemMode {FilterMode,ShowMode};

	VParamFilterMenu(QMenu* parent,VParamSet* filter,QString title,ItemMode,DecorMode decorMode=NoDecor);
	void reload();

protected Q_SLOTS:
	void slotChanged(bool);
	void slotSelectAll(bool);
	void slotUnselectAll(bool);

protected:
	void buildTitle(QString,QMenu*);
	void addAction(QString name,QString id);
    void checkActionState();

	QMenu*  menu_;
	VParamSet* filter_;
	ItemMode itemMode_;
	DecorMode decorMode_;
    QAction *selectAllAc_;
    QAction *unselectAllAc_;
};


class ServerFilterMenu : public QObject, public ServerListObserver, public ServerFilterObserver
{
Q_OBJECT

public:
	explicit ServerFilterMenu(QMenu* parent);
	~ServerFilterMenu() override;

	void reload(ServerFilter*);
	void aboutToDestroy(); //Called when the parent mainwindow is being destroyed

	//From ServerListObserver
	void notifyServerListChanged() override;
	void notifyServerListFavouriteChanged(ServerItem*) override;

	//From ConfigObserver
	void notifyServerFilterAdded(ServerItem*) override;
	void notifyServerFilterRemoved(ServerItem*) override;
	void notifyServerFilterChanged(ServerItem*) override;
	void notifyServerFilterDelete() override;

protected Q_SLOTS:
	void slotChanged(bool);

protected:
	void init();
	void clear();
	QAction* createAction(QString name,int id);
	void reload();
	void buildFavourite();
	void clearFavourite();
	void syncActionState(QString,bool);

	QMenu*  menu_;
	QMenu*  allMenu_;
	QMap<QString,QAction*> acAllMap_;
	QMap<QString,QAction*> acFavMap_;
	ServerFilter* filter_;
	QFont font_;
	QFont loadFont_;
};

#endif
