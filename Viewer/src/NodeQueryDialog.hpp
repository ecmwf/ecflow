#ifndef NODEQUERYDIALOG_HPP_
#define NODEQUERYDIALOG_HPP_

#include <QDialog>
#include <QAbstractItemModel>

#include "ServerFilter.hpp"

#include "ui_NodeQueryDialog.h"
#include "ui_NodeQuerySaveDialog.h"

class NodeQuery;
class NodeQueryEngine;
class NodeQueryListModel;
class NodeQueryModel;
class QSortFilterProxyModel;

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


class NodeQueryDialog : public QDialog, protected Ui::NodeQueryDialog, public ServerFilterObserver
{
    Q_OBJECT

public:
    explicit NodeQueryDialog(QWidget *parent = 0);
    ~NodeQueryDialog();

    void setServerFilter(ServerFilter*);

    void notifyServerFilterAdded(ServerItem*);
    void notifyServerFilterRemoved(ServerItem*);
    void notifyServerFilterChanged(ServerItem*);
    void notifyServerFilterDelete();

protected Q_SLOTS:
	void accept();
    void reject();
    void buildQueryString();
	void slotTypeCbChanged();
	void slotStateCbChanged();
	void slotFlagCbChanged();
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

protected:
	void closeEvent(QCloseEvent * event);

private:
	void updateServers();
	void initAttr();
	void updateQuery();
    void readSettings();
    void writeSettings();

	NodeQuery* query_;
	NodeQueryEngine* engine_;
	NodeQueryModel* model_;
	QSortFilterProxyModel* sortModel_;

	NodeQueryListModel* queryListModel_;
	ServerFilter* serverFilter_;
};

class NodeQueryModel : public QAbstractItemModel
{
 Q_OBJECT
public:
   	explicit NodeQueryModel(QObject *parent=0);
   	~NodeQueryModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	void clearData();

public Q_SLOTS:
	void appendRow(QStringList);

protected:
	QList<QStringList> data_;
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





#endif
