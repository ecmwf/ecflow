//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ChangeNotifyModel.hpp"

#include "VNode.hpp"
#include "VNodeList.hpp"

#include <QDebug>

ChangeNotifyModel::ChangeNotifyModel(QObject *parent) :
     QAbstractItemModel(parent),
     data_(0)
{
}

ChangeNotifyModel::~ChangeNotifyModel()
{
}

VNodeList* ChangeNotifyModel::data()
{
	return data_;
}

void ChangeNotifyModel::setData(VNodeList *data)
{
	beginResetModel();

	data_=data;

	connect(data_,SIGNAL(beginAppendRow()),
			this,SLOT(slotBeginAppendRow()));

	connect(data_,SIGNAL(endAppendRow()),
			this,SLOT(slotEndAppendRow()));

	connect(data_,SIGNAL(beginRemoveRow(int)),
			this,SLOT(slotBeginRemoveRow(int)));

	connect(data_,SIGNAL(endRemoveRow(int)),
			this,SLOT(slotEndRemoveRow(int)));

	connect(data_,SIGNAL(beginRemoveRows(int,int)),
			this,SLOT(slotBeginRemoveRows(int,int)));

	connect(data_,SIGNAL(endRemoveRows(int,int)),
			this,SLOT(slotEndRemoveRows(int,int)));

	connect(data_,SIGNAL(beginReset()),
			this,SLOT(slotBeginReset()));

	connect(data_,SIGNAL(endReset()),
			this,SLOT(slotEndReset()));

	endResetModel();
}

bool ChangeNotifyModel::hasData() const
{
	return (data_ && data_->size() >0);
}

int ChangeNotifyModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
}

int ChangeNotifyModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	//Parent is the root:
	if(!parent.isValid())
	{
		return data_->size();
	}

	return 0;
}


QVariant ChangeNotifyModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid() || !hasData())
    {
		return QVariant();
	}
	int row=index.row();
	if(row < 0 || row >= data_->size())
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		VNodeListItem *item=data_->itemAt(row);
		assert(item);

		switch(index.column())
		{
		case 0:
			return QString::fromStdString(item->server());
			break;
		case 1:
			return QString::fromStdString(item->path());
			break;
		case 2:
			return item->time();
			break;
		default:
			break;
		}
	}

	return QVariant();
}

QVariant ChangeNotifyModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
      		  return QAbstractItemModel::headerData( section, orient, role );

   	if(role == Qt::DisplayRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Server");
   		case 1: return tr("Node");
   		case 2: return tr("Time of change");
   		default: return QVariant();
   		}
   	}
   	else if(role== Qt::ToolTipRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Server");
   		case 1: return tr("Node");
   		case 2: return tr("Time of change");
   		default: return QVariant();
   		}
   	}
    return QVariant();
}

QModelIndex ChangeNotifyModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return QModelIndex();
	}

	//When parent is the root this index refers to a node or server
	if(!parent.isValid())
	{
		return createIndex(row,column);
	}

	return QModelIndex();

}

QModelIndex ChangeNotifyModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

VInfo_ptr ChangeNotifyModel::nodeInfo(const QModelIndex& index) const
{
    VInfo_ptr res;

    if(!index.isValid() || !hasData())
    {
        return res;
    }

    int row=index.row();
    if(row < 0 || row >= data_->size())
        return res;

    VNodeListItem *item=data_->itemAt(row);
    Q_ASSERT(item);
    VNode* vnode=item->node();
    Q_ASSERT(vnode);

    if(vnode->isServer())
        return VInfoServer::create(vnode->server());
    else
        return VInfoNode::create(vnode);

    return res;
}

void ChangeNotifyModel::slotBeginAppendRow()
{
	beginInsertRows(QModelIndex(),data_->size(),data_->size());
}

void ChangeNotifyModel::slotEndAppendRow()
{
	endInsertRows();
}

void ChangeNotifyModel::slotBeginRemoveRow(int row)
{
	beginRemoveRows(QModelIndex(),row,row);
}

void ChangeNotifyModel::slotEndRemoveRow(int row)
{
	endRemoveRows();
}

void ChangeNotifyModel::slotBeginRemoveRows(int rowStart,int rowEnd)
{
	beginRemoveRows(QModelIndex(),rowStart,rowEnd);
}

void ChangeNotifyModel::slotEndRemoveRows(int,int)
{
	endRemoveRows();
}


void ChangeNotifyModel::slotBeginReset()
{
	beginResetModel();
}

void ChangeNotifyModel::slotEndReset()
{
	endResetModel();
}


