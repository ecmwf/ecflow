//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ModelColumn.hpp"
#include "ServerHandler.hpp"
#include "VNode.hpp"

#include <QDebug>
#include <QTime>
#include "NodeQueryResultModel.hpp"

NodeQueryResultModel::NodeQueryResultModel(QObject *parent) :
          QAbstractItemModel(parent),
          columns_(0)
{
	columns_=ModelColumn::def("query_columns");

	assert(columns_);
}

NodeQueryResultModel::~NodeQueryResultModel()
{
	Q_FOREACH(NodeQueryResultData* d,data_)
	{
		delete d;
	}
}

void  NodeQueryResultModel::appendRow(NodeQueryResultData dInput)
{
	int num=data_.count();
	Q_EMIT beginInsertRows(QModelIndex(),num,num);

	NodeQueryResultData* d=new NodeQueryResultData(dInput);
	data_ << d;

	Q_EMIT endInsertRows();
}

void  NodeQueryResultModel::appendRows(QList<NodeQueryResultData> dInput)
{
	if(dInput.isEmpty())
		return;

	int num=data_.count();
	Q_EMIT beginInsertRows(QModelIndex(),num,num+dInput.count()-1);

	for(int i=0; i < dInput.count(); i++)
	{
		NodeQueryResultData* d=new NodeQueryResultData(dInput.at(i));
		data_ << d;
	}

	Q_EMIT endInsertRows();
}

void NodeQueryResultModel::clearData()
{
	beginResetModel();

	Q_FOREACH(NodeQueryResultData* d,data_)
	{
		delete d;
	}
	data_.clear();

	endResetModel();
}

bool NodeQueryResultModel::hasData() const
{
	return !data_.isEmpty();
}

int NodeQueryResultModel::columnCount( const QModelIndex& /*parent */) const
{
   	 return columns_->count();
}

int NodeQueryResultModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	//Parent is the root:
	if(!parent.isValid())
	{
		return data_.count();
	}

	return 0;
}

Qt::ItemFlags NodeQueryResultModel::flags ( const QModelIndex & index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant NodeQueryResultModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid() || !hasData() ||
	   (role != Qt::DisplayRole && role != Qt::BackgroundRole && role != Qt::TextAlignmentRole))
    {
		return QVariant();
	}

	int row=index.row();
	if(row < 0 || row >= data_.size())
		return QVariant();

	QString id=columns_->id(index.column());

	NodeQueryResultData* d=data_.at(row);
	VNode* node=d->node_;

	if(role == Qt::DisplayRole)
	{
		if(id == "path")
		{
			return QString::fromStdString(node->absNodePath());
		}
		else if(id == "server")
		{
			if(node->server())
				return QString::fromStdString(node->server()->name());
		}
		else if(id == "type")
			return QString::fromStdString(node->nodeType());
		else if(id == "status")
			return node->stateName();

		return QVariant();
	}
	else if(role == Qt::BackgroundRole)
	{
		if(id == "status")
			return node->stateColour();

		return QVariant();
	}
	else if(role == Qt::TextAlignmentRole)
	{
		if(id == "status" || id == "type")
			return Qt::AlignCenter;

		return QVariant();
	}

	return QVariant();
}

QVariant NodeQueryResultModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal)
      		  return QAbstractItemModel::headerData( section, orient, role );

	QString id=columns_->id(section);

	if(role == Qt::DisplayRole)
		return columns_->label(section);
	else if(role == Qt::UserRole)
		return columns_->id(section);
	else if(role == Qt::ToolTipRole)
		return columns_->tooltip(section);
	else if(role == Qt::TextAlignmentRole)
	{
		if(id == "status" || id == "type")
			return Qt::AlignCenter;
	}

	return QVariant();
}

QModelIndex NodeQueryResultModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex NodeQueryResultModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

VInfo_ptr NodeQueryResultModel::nodeInfo(const QModelIndex& index)
{
	//For invalid index no info is created.
	if(!index.isValid())
	{
		VInfo_ptr res;
		return res;
	}

	if(index.row() >=0 && index.row() <= data_.count())
	{
		NodeQueryResultData* d=data_.at(index.row());

		/*if(ServerHandler *s=ServerHandler::find(d->server_.toStdString()))
		{
			if(d->path_.isEmpty() || d->path_ == "/")
			{
				return VInfoServer::create(s);
			}

			if(VNode* node=s->vRoot()->find(d->path_.toStdString()))
			{
				return VInfoNode::create(node);
			}
		}*/
	}

    VInfo_ptr res;
	return res;
}

QModelIndex NodeQueryResultModel::infoToIndex(VInfo_ptr info)
{
	/*if(info && info.get())
	{
		if(info->isServer())
		{
			if(ServerHandler *s=info->server())
			{
				return serverToIndex(s);
			}
		}
		else if(VNode* n=info->node())
		{
				return nodeToIndex(n);
		}
	}*/

	return QModelIndex();
}










