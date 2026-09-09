/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_FileWidget_HPP
#define ecflow_viewer_FileWidget_HPP

#include <QMap>
#include <QMenu>
#include <QSet>
#include <QWidget>

#include "ServerFilter.hpp"
#include "ServerList.hpp"
#include "ecflow/core/DState.hpp"

class QToolButton;
class VParam;
class VParamSet;
class ServerFilter;

class VParamFilterMenu : public QObject {
    Q_OBJECT

public:
    enum DecorMode { NoDecor, ColourDecor, PixmapDecor };
    enum ItemMode { FilterMode, ShowMode };

    VParamFilterMenu(QMenu* parent, VParamSet* filter, QString title, ItemMode, DecorMode decorMode = NoDecor);
    void reload();

protected Q_SLOTS:
    void slotChanged(bool);
    void slotSelectAll(bool);
    void slotUnselectAll(bool);

protected:
    void buildTitle(QString, QMenu*);
    void addAction(QString name, QString id);
    void checkActionState();

    QMenu* menu_;
    VParamSet* filter_;
    ItemMode itemMode_;
    DecorMode decorMode_;
    QAction* selectAllAc_;
    QAction* unselectAllAc_;
};

class ServerFilterMenu : public QObject, public ServerListObserver, public ServerFilterObserver {
    Q_OBJECT

public:
    explicit ServerFilterMenu(QMenu* parent);
    ~ServerFilterMenu() override;

    void reload(ServerFilter*);
    void aboutToDestroy(); // Called when the parent mainwindow is being destroyed

    // From ServerListObserver
    void notifyServerListChanged() override;
    void notifyServerListFavouriteChanged(ServerItem*) override;

    // From ConfigObserver
    void notifyServerFilterAdded(ServerItem*) override;
    void notifyServerFilterRemoved(ServerItem*) override;
    void notifyServerFilterChanged(ServerItem*) override;
    void notifyServerFilterDelete() override;

protected Q_SLOTS:
    void slotChanged(bool);

protected:
    void init();
    void clear();
    QAction* createAction(QString name, int id);
    void reload(bool favoriteBuilt);
    void buildFavourite();
    void clearFavourite();
    void syncActionState(QString, bool);

    QMenu* menu_;
    QMenu* allMenu_;
    QMap<QString, QAction*> acAllMap_;
    QMap<QString, QAction*> acFavMap_;
    ServerFilter* filter_;
    QFont font_;
    QFont loadFont_;
};

#endif /* ecflow_viewer_FileWidget_HPP */
