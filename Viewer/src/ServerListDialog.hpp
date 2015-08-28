//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLISTDIALOG_HPP_
#define SERVERLISTDIALOG_HPP_

#include "ui_ServerListDialog.h"
#include "ui_ServerEditDialog.h"
#include "ui_ServerAddDialog.h"

class QSortFilterProxyModel;
class ServerFilter;
class ServerItem;
class ServerListModel;

class ServerDialogChecker
{
protected:
	explicit ServerDialogChecker(QString txt) : errorText_(txt) {};

    bool checkName(QString name,QString oriName=QString());
	bool checkHost(QString host);
	bool checkPort(QString port);
	void error(QString msg);

	QString errorText_;
};


class ServerEditDialog : public QDialog, private Ui::ServerEditDialog, public ServerDialogChecker
{
Q_OBJECT

public:
	ServerEditDialog(QString name,QString host, QString port,QWidget* parent=0);

	QString name() const;
	QString host() const;
	QString port() const;

public Q_SLOTS:
	void accept();

private:
    QString oriName_;

};

class ServerAddDialog : public QDialog, private Ui::ServerAddDialog, public ServerDialogChecker
{
Q_OBJECT

public:
	explicit ServerAddDialog(QWidget* parent=0);

	QString name() const;
	QString host() const;
	QString port() const;
	bool addToView() const;

public Q_SLOTS:
	void accept();
};


class ServerListDialog : public QDialog, protected Ui::ServerListDialog
{
Q_OBJECT

public:
	enum Mode {SelectionMode,ManageMode};

	ServerListDialog(Mode,ServerFilter*,QWidget *parent=0);
	~ServerListDialog();

public Q_SLOTS:
	   void accept();

protected Q_SLOTS:
	 void on_actionEdit_triggered();
	 void on_actionAdd_triggered();
	 void on_actionDuplicate_triggered();
	 void on_actionDelete_triggered();
	 void on_actionRescan_triggered();
	 void on_serverView_doubleClicked(const QModelIndex& index);

protected:
	void editItem(const QModelIndex& index);
	void duplicateItem(const QModelIndex& index);
	void addItem();
	void removeItem(const QModelIndex& index);
	void writeSettings();
	void readSettings();

	ServerFilter* filter_;
	ServerListModel* model_;
	QSortFilterProxyModel* sortModel_;
	Mode mode_;
};

class ServerListModel : public QAbstractItemModel
{
public:
	explicit ServerListModel(QObject *parent=0);
	~ServerListModel();

	virtual int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	virtual int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	virtual QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	virtual bool setData( const QModelIndex &, const QVariant &, int role = Qt::EditRole );
	virtual QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	void dataIsAboutToChange();
   	void dataChangeFinished();
   	ServerItem* indexToServer(const QModelIndex& index);
};

#endif

