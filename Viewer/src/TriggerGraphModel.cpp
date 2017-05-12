//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerGraphModel.hpp"

#include "ModelColumn.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

#include <QDebug>

TriggerGraphModel::TriggerGraphModel(QObject *parent) :
          QAbstractItemModel(parent),
          tc_(0),
          columns_(0)
{
    columns_=ModelColumn::def("trigger_graph_columns");
    assert(columns_);
/*
	data_=new NodeQueryResult(this);

	connect(data_,SIGNAL(beginAppendRow()),
			this,SLOT(slotBeginAppendRow()));

	connect(data_,SIGNAL(endAppendRow()),
			this,SLOT(slotEndAppendRow()));

	connect(data_,SIGNAL(beginAppendRows(int)),
			this,SLOT(slotBeginAppendRows(int)));

	connect(data_,SIGNAL(endAppendRows(int)),
			this,SLOT(slotEndAppendRows(int)));

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

	connect(data_,SIGNAL(stateChanged(const VNode*,int,int)),
			this,SLOT(slotStateChanged(const VNode*,int,int)));
*/
}

TriggerGraphModel::~TriggerGraphModel()
{
}


void TriggerGraphModel::clearData()
{
    beginResetModel();
    tc_=0;
    endResetModel();
}

void TriggerGraphModel::beginUpdate()
{
	beginResetModel();
}

void TriggerGraphModel::endUpdate()
{
	endResetModel();
}


void TriggerGraphModel::setTriggerCollector(TriggerListCollector *tc)
{
    beginResetModel();
    tc_ = tc;
    endResetModel();
}

bool TriggerGraphModel::hasData() const
{
    if(tc_)
		return tc_->size() > 0;
	else
		return false;
}

int TriggerGraphModel::columnCount( const QModelIndex& /*parent */) const
{
     return 1 ;//columns_->count();
}

int TriggerGraphModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	//Parent is the root:
	if(!parent.isValid())
	{
		return tc_->size();
	}

	return 0;
}

Qt::ItemFlags TriggerGraphModel::flags ( const QModelIndex & index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TriggerGraphModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid() || !hasData() ||
	   (role != Qt::DisplayRole && role != Qt::BackgroundRole && role != Qt::TextAlignmentRole))
    {
		return QVariant();
	}

	int row=index.row();
	if(row < 0 || row >= tc_->size())
		return QVariant();

    //QString id=columns_->id(index.column());

	const std::vector<TriggerListItem*>& items=tc_->items();
	VItem *t=items[row]->item();
    Q_ASSERT(t);

    if(VAttribute* a=t->isAttribute())
    {
        if(role == Qt::DisplayRole)
            return a->data();
    }
    else if(VNode* vnode=t->isNode())
    {
        if(role == Qt::DisplayRole)
        {
            return QString::fromStdString(vnode->absNodePath());
        }
        else if(role == Qt::BackgroundRole)
        {
            if(vnode->isSuspended())
            {
                QVariantList lst;
                lst << vnode->stateColour() << vnode->realStateColour();
                return lst;
            }
            else
                return vnode->stateColour() ;
        }
        else if(role == Qt::ForegroundRole)
            return vnode->stateFontColour();
    }

#if 0
        if(id == "path")
			return "PATH"; //d->pathStr();
		else if(id == "server")
			return "SERVER"; //d->serverStr();
		else if(id == "type")
			return "TYPE"; //d->typeStr();
		else if(id == "status")
			return "STATUS"; //d->stateStr();
        else if(id == "attribute")
            return "ATTR"; //d->attr();
        else if(id == "trigger")
            return QString::fromStdString(t->fullPath());

        return QVariant();
#endif

#if 0
    else if(role == Qt::BackgroundRole)
	{
		//if(id == "status")
		//	return d->stateColour();

		return QVariant();
	}
#endif

	return QVariant();
}

QVariant TriggerGraphModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal)
      		  return QAbstractItemModel::headerData( section, orient, role );

#if 0
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
#endif
	return QVariant();
}

QModelIndex TriggerGraphModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex TriggerGraphModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

VInfo_ptr TriggerGraphModel::nodeInfo(const QModelIndex& index)
{
	//For invalid index no info is created.
	if(!index.isValid())
	{
		VInfo_ptr res;
		return res;
	}
/*
	if(index.row() >=0 && index.row() <= tc_->items().size())
	{
		TriggerListItem* d=tc_->items()[index.row()];
		VItem *vitem = d->item();
		vitem->isServer()
				vitem->

		if(ServerHandler *s=d->server_)
		{
			if(d->node_)
			{
                //server
				if(d->node_->isServer())
				{
					return VInfoServer::create(s);
				}
                //node
                else if(!d->hasAttribute())
				{
					return VInfoNode::create(d->node_);
				}
                //attribute
                else
                {
                    if(VAttribute* a=d->node_->findAttribute(d->attr_))
                        return VInfoAttribute::create(a);
                    else
                        return VInfoNode::create(d->node_);
                }
			}
		}
	}
*/
    VInfo_ptr res;
	return res;
}

QModelIndex TriggerGraphModel::infoToIndex(VInfo_ptr info)
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

void  TriggerGraphModel::slotBeginAppendRow()
{
	int num=tc_->items().size();
	Q_EMIT beginInsertRows(QModelIndex(),num,num);


	Q_EMIT endInsertRows();
}

void  TriggerGraphModel::slotEndAppendRow()
{
	Q_EMIT endInsertRows();
}

void  TriggerGraphModel::slotBeginAppendRows(int n)
{
	if(n <= 0)
		return;

	int num=tc_->items().size();
	Q_EMIT beginInsertRows(QModelIndex(),num,num+n-1);
}

void  TriggerGraphModel::slotEndAppendRows(int n)
{
	if(n <= 0)
		return;
	Q_EMIT endInsertRows();
}

void TriggerGraphModel::slotBeginRemoveRow(int row)
{
	beginRemoveRows(QModelIndex(),row,row);
}

void TriggerGraphModel::slotEndRemoveRow(int row)
{
	endRemoveRows();
}

void TriggerGraphModel::slotBeginRemoveRows(int rowStart,int rowEnd)
{
	beginRemoveRows(QModelIndex(),rowStart,rowEnd);
}

void TriggerGraphModel::slotEndRemoveRows(int,int)
{
	endRemoveRows();
}

void TriggerGraphModel::slotBeginReset()
{
	beginResetModel();
}

void TriggerGraphModel::slotEndReset()
{
	endResetModel();
}

void TriggerGraphModel::slotStateChanged(const VNode*,int pos,int cnt)
{
	int col=columns_->indexOf("status");

	if(col != -1)
	{
		QModelIndex fromIdx=index(pos,col);
		QModelIndex toIdx=index(pos+cnt-1,col);

		Q_EMIT dataChanged(fromIdx,toIdx);
	}
}
