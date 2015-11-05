//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_NODEQUERYWIDGET_HPP_
#define VIEWER_SRC_NODEQUERYWIDGET_HPP_

#include <QAbstractItemModel>
#include <QDialog>
#include <QWidget>

#include "ServerFilter.hpp"
#include "VInfo.hpp"

#include "ui_NodeQueryWidget.h"
#include "ui_NodeQuerySaveDialog.h"

class NodeQuery;
class NodeQueryEngine;
class NodeQueryListModel;
class NodeQueryResultModel;

class NodeQuerySaveDialog : public QDialog, protected Ui::NodeQuerySaveDialog
{
Q_OBJECT

public:
    explicit NodeQuerySaveDialog(QWidget *parent = 0);
    ~NodeQuerySaveDialog() {};
    QString name() const;

public Q_SLOTS:
	void accept();
};


class NodeQueryWidget : public QWidget, protected Ui::NodeQueryWidget, public ServerFilterObserver
{
    Q_OBJECT

public:
    explicit NodeQueryWidget(QWidget *parent = 0);
    ~NodeQueryWidget();

    void setServerFilter(ServerFilter*);

    void notifyServerFilterAdded(ServerItem*);
    void notifyServerFilterRemoved(ServerItem*);
    void notifyServerFilterChanged(ServerItem*);
    void notifyServerFilterDelete();

protected Q_SLOTS:
	//void accept();
    //void reject();
	void slotShowDefPanel(bool);
	void buildQueryString();
	void slotServerCbChanged();
	void slotTypeListChanged();
	void slotStateListChanged();
	void slotFlagListChanged();
	void slotAttrListChanged();
	void slotSearchTermEdited(QString);
	void slotRootNodeEdited(QString);
	void slotExactMatch(bool);
	void slotFind();
	void check();
	void slotStop();
	void slotAddResult(QStringList);
	void slotQueryStarted();
	void slotQueryFinished();
	void slotSaveQueryAs();

Q_SIGNALS:
	void closeClicked();
	void selectionChanged(VInfo_ptr);

private:
	void updateServers();
	void initAttr();
	void updateQuery();

	NodeQuery* query_;
	NodeQueryEngine* engine_;
	NodeQueryResultModel* model_;

	NodeQueryListModel* queryListModel_;
	ServerFilter* serverFilter_;
};

class NodeQueryListModel : public QAbstractItemModel
{
public:
   	explicit NodeQueryListModel(QObject *parent=0);
   	~NodeQueryListModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	void updateData();

protected:
	std::vector<NodeQuery*> data_;
};


#endif /* VIEWER_SRC_NODEQUERYWIDGET_HPP_ */
