//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "TriggerTableModel.hpp"

#include "IconProvider.hpp"
#include "ModelColumn.hpp"
#include "ServerHandler.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VIcon.hpp"
#include "VNode.hpp"

#include <QDebug>

TriggerTableModel::TriggerTableModel(Mode mode,QObject *parent) :
          QAbstractItemModel(parent),
          tc_(0),
          columns_(0),
          mode_(mode)
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

TriggerTableModel::~TriggerTableModel()
{
}


void TriggerTableModel::clearData()
{
    beginResetModel();
    tc_=0;
    endResetModel();
}

void TriggerTableModel::beginUpdate()
{
	beginResetModel();
}

void TriggerTableModel::endUpdate()
{
	endResetModel();
}


void TriggerTableModel::setTriggerCollector(TriggerTableCollector *tc)
{
    beginResetModel();
    tc_ = tc;
    endResetModel();
}

bool TriggerTableModel::hasData() const
{
    if(tc_)
		return tc_->size() > 0;
	else
		return false;
}

int TriggerTableModel::columnCount( const QModelIndex& /*parent */) const
{
     return 1;
}

int TriggerTableModel::rowCount( const QModelIndex& parent) const
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

Qt::ItemFlags TriggerTableModel::flags ( const QModelIndex & index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TriggerTableModel::data( const QModelIndex& index, int role ) const
{   
    if(!index.isValid() || !hasData() || (role < Qt::UserRole &&
        role != Qt::DisplayRole && role != Qt::BackgroundRole && role != Qt::TextAlignmentRole &&
        role != Qt::ToolTipRole && role != Qt::ForegroundRole))
    {
		return QVariant();
	}

	int row=index.row();
	if(row < 0 || row >= tc_->size())
		return QVariant();

    //QString id=columns_->id(index.column());

    const std::vector<TriggerTableItem*>& items=tc_->items();
	VItem *t=items[row]->item();
    Q_ASSERT(t);

#if 0
    if(index.column() == 0)
    {
        if(role == Qt::DisplayRole)
        {
            const std::set<TriggerCollector::Mode>&  modes=items[row]->modes();
            std::string s;
            s+=(modes.find(TriggerCollector::Normal) != modes.end())?"d":"";
            s+=(modes.find(TriggerCollector::Parent) != modes.end())?"p":"";
            s+=(modes.find(TriggerCollector::Child) != modes.end())?"c":"";
            return QString::fromStdString(s);
        }
        else if (role == Qt::ForegroundRole)
        {
            return QColor(90,90,90);
        }
        else if (role == Qt::ToolTipRole)
        {
            return QVariant();
        }
    }
#endif
    if(index.column() == 0)
    {
        if(VAttribute* a=t->isAttribute())
        {
            if(role == Qt::DisplayRole)
            {
                QStringList d=a->data();
                if(VNode* pn=a->parent())
                    d.append(QString::fromStdString(pn->absNodePath()));
                return d;
            }
            else if(role ==  Qt::ToolTipRole)
            {
                return a->toolTip();
            }

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

            else if(role == Qt::ToolTipRole)
            {
                QString txt=vnode->toolTip();
                //txt+=VIcon::toolTip(vnode,icons_);
                return txt;
            }
            else if(role == IconRole)
            {
                return VIcon::pixmapList(vnode,0);
            }
            else if(role  == NodeTypeRole)
            {
                 if(vnode->isTask()) return 2;
                 else if(vnode->isSuite()) return 0;
                 else if(vnode->isFamily()) return 1;
                 else if(vnode->isAlias()) return 3;
                 return 0;
            }
            else if(role  == NodeTypeForegroundRole)
            {
                return vnode->typeFontColour();
            }

        }
    }

    //We express the table cell background colour through the UserRole. The
    //BackgroundRole is already used for the node rendering
    if(role == Qt::UserRole)
    {
        const std::set<TriggerCollector::Mode>&  modes=items[row]->modes();
        if(modes.find(TriggerCollector::Normal) != modes.end())
            return QVariant();
#if 0
        else if(modes.find(TriggerCollector::Parent) != modes.end())
            //return QColor(244,252,255);
            return QColor(251,250,250);
        else if(modes.find(TriggerCollector::Child) != modes.end())
            //return QColor(237,249,255);
#endif
        else
            return QColor(239,240,240);
    }


	return QVariant();
}

QVariant TriggerTableModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal)
            return QAbstractItemModel::headerData( section, orient, role );

	return QVariant();
}

QModelIndex TriggerTableModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex TriggerTableModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

VInfo_ptr TriggerTableModel::nodeInfo(const QModelIndex& index)
{
	//For invalid index no info is created.
	if(!index.isValid())
	{
		VInfo_ptr res;
		return res;
	}

	if(index.row() >=0 && index.row() <= tc_->items().size())
	{
        TriggerTableItem* d=tc_->items()[index.row()];
        return VInfo::createFromItem(d->item());
    }

    VInfo_ptr res;
	return res;
}

QModelIndex TriggerTableModel::infoToIndex(VInfo_ptr info)
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

TriggerTableItem* TriggerTableModel::indexToItem(const QModelIndex& index) const
{
    if(!hasData())
        return 0;

    int row=index.row();
    if(row < 0 || row >= tc_->size())
        return 0;

    const std::vector<TriggerTableItem*>& items=tc_->items();
    return items[row];
}

void  TriggerTableModel::slotBeginAppendRow()
{
	int num=tc_->items().size();
	Q_EMIT beginInsertRows(QModelIndex(),num,num);


	Q_EMIT endInsertRows();
}

void  TriggerTableModel::slotEndAppendRow()
{
	Q_EMIT endInsertRows();
}

void  TriggerTableModel::slotBeginAppendRows(int n)
{
	if(n <= 0)
		return;

	int num=tc_->items().size();
	Q_EMIT beginInsertRows(QModelIndex(),num,num+n-1);
}

void  TriggerTableModel::slotEndAppendRows(int n)
{
	if(n <= 0)
		return;
	Q_EMIT endInsertRows();
}

void TriggerTableModel::slotBeginRemoveRow(int row)
{
	beginRemoveRows(QModelIndex(),row,row);
}

void TriggerTableModel::slotEndRemoveRow(int row)
{
	endRemoveRows();
}

void TriggerTableModel::slotBeginRemoveRows(int rowStart,int rowEnd)
{
	beginRemoveRows(QModelIndex(),rowStart,rowEnd);
}

void TriggerTableModel::slotEndRemoveRows(int,int)
{
	endRemoveRows();
}

void TriggerTableModel::slotBeginReset()
{
	beginResetModel();
}

void TriggerTableModel::slotEndReset()
{
	endResetModel();
}

void TriggerTableModel::slotStateChanged(const VNode*,int pos,int cnt)
{
	int col=columns_->indexOf("status");

	if(col != -1)
	{
		QModelIndex fromIdx=index(pos,col);
		QModelIndex toIdx=index(pos+cnt-1,col);

		Q_EMIT dataChanged(fromIdx,toIdx);
	}
}
