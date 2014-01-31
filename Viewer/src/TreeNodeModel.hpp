#ifndef TREENODEMODEL_H
#define TREENODEMODEL_H

#include <QAbstractItemModel>

#include <vector>

class Node;
class ServerHandler;

class TreeNodeModel : public QAbstractItemModel
{

public:
   	TreeNodeModel(QObject *parent=0);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	bool hasData() const;
	void addServer(ServerHandler *);
	void dataIsAboutToChange();
	//void setRootNode(MvQOgcNode*);
	bool isServer(const QModelIndex & index) const;
	ServerHandler* indexToServer(const QModelIndex & index) const;

	QModelIndex indexFromNode(Node*) const;
	Node* nodeFromIndex( const QModelIndex & index) const;

protected:
	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
	QList<ServerHandler*> servers_;

};

#endif
