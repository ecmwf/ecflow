//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERWIDGET_HPP_
#define SERVERWIDGET_HPP_

#include <QAbstractItemModel>
#include <QDialog>

#include "ui_ServerDialog.h"
#include "ui_ServerItemDialog.h"

class QSortFilterProxyModel;

class ServerFilter;
class ServerItem;

class ServerSelectionItem
{
public:
	ServerSelectionItem(ServerItem* s,bool sel) : server_(s), selected_(sel) {};
	~ServerSelectionItem();

	ServerItem* server() const {return server_;}
	bool selected() const {return selected_;}
	void selected(bool b) {selected_=b;}

protected:
	ServerItem* server_;
	bool selected_;
};

class ServerSelectionData
{
public:
	ServerSelectionData(ServerFilter *);
	~ServerSelectionData();

	void add(ServerItem *server,bool selection);
	void selectedServers(std::vector<ServerItem*>&);
	void allServers(std::vector<ServerItem*>&);
	bool isSelected(int) const;
	void selected(int,bool);
	ServerItem* server(int) const;

	void add();
	void remove(ServerItem *server);
	int count() {return static_cast<int>(items_.size());}

protected:
	std::vector<ServerSelectionItem*> items_;
};


class ServerSelectionModel : public QAbstractItemModel
{
public:
	ServerSelectionModel(ServerSelectionData*,QObject *parent=0);
	~ServerSelectionModel();

	virtual int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	virtual int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	virtual QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	virtual bool setData( const QModelIndex &, const QVariant &, int role = Qt::EditRole );
	virtual QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void dataIsAboutToChange();
	void dataChangeFinished();

	Qt::ItemFlags flags ( const QModelIndex &) const;
	ServerItem* indexToServer(const QModelIndex& index);

protected:
	ServerSelectionData *data_;
};

class ServerItemDialog : public QDialog, private Ui::ServerItemDialog
{
   Q_OBJECT

public:
	ServerItemDialog(ServerItem *,QWidget* parent=0);

public slots:
	   void accept();
	   void reject();

protected:
	   void error(QString);
	   ServerItem* item_;
};

class ServerDialog : public QDialog, private Ui::ServerDialog
{
   Q_OBJECT

public:
	ServerDialog(ServerFilter *,QWidget* parent=0);
	~ServerDialog();
	//void setServerFilter(ServerFilter *filter);

public slots:
   void accept();
   void reject();

protected slots:
   	void on_editTb_clicked();
   	void on_addTb_clicked();
   	void on_deleteTb_clicked();
   	void on_rescanTb_clicked();
   	void on_serverView_doubleClicked(const QModelIndex& index);

protected:
   	void editItem(const QModelIndex& index);
    void addItem();
    void removeItem(const QModelIndex& index);
   	void saveSelection();
    void readSettings();
	void writeSettings();

	ServerFilter* filter_;
	ServerSelectionModel* model_;
	QSortFilterProxyModel *sortModel_;
	ServerSelectionData* data_;
};

#endif
