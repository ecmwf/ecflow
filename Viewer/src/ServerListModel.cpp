//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerListModel.hpp"

#include "ServerFilter.hpp"
#include "ServerItem.hpp"
#include "ServerList.hpp"

#include <QDebug>

//======================================
//
// ServerSelectionData
//
//======================================

ServerSelectionData::ServerSelectionData(ServerFilter * filter)
{
	for(int i=0; i < ServerList::Instance()->count(); i++)
	{
		items_.push_back(ServerModelItem(ServerList::Instance()->item(i)->clone(),false));
	}

	if(filter)
	{
		for(unsigned int i=0; i < filter->servers().size(); i++)
		{
			bool found=false;
			for(std::vector<ServerModelItem>::iterator it=items_.begin(); it !=items_.end(); it++)
			{
						if((*it).server->match(filter->servers().at(i)))
						{
							(*it).status=true;
							found = true;
							break;
						}
				}
				if(!found)
				{
					items_.push_back(ServerModelItem(filter->servers().at(i)->clone(),true));
				}
			}
		}
}

ServerItem* ServerSelectionData::server(int index)
{
	if(index >=0 && index < items_.size())
		return items_.at(index).server;
	return 0;
}

bool ServerSelectionData::selected(int index)
{
	if(index >=0 && index < items_.size())
		return items_.at(index).status;
	return 0;
}

//======================================
//
// ServerListModel
//
//======================================

ServerListModel::ServerListModel(ServerFilter * filter,QObject *parent) :
  QAbstractItemModel(parent)
{
	data_=new ServerModelData(filter);
		/*
		const std::vector<ServerFilterItem*>& sv=filter->servers();
		for(std::vector<ServerFilterItem*>::const_iterator it=filter->servers().begin(); it != filter->servers().end(); it++)
		{
			const ServerFilterItem* s=(*it);
			selection_.push_back(s->clone());
		}*/
	}
}

ServerListModel::~ServerListModel()
{
	delete data_;
}

int ServerListModel::columnCount(const QModelIndex& parent) const
{
	return 3;
}

int ServerListModel::rowCount(const QModelIndex& parent) const
{
	if(!parent.isValid())
		return data_->count();

	return 0;
}

QVariant ServerListModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid() ||
	   (role != Qt::DisplayRole && role != Qt::CheckStateRole))
	{
		return QVariant();

	}

	ServerItem* item=data_->server(index.row());
	if(!item)
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0: return QString::fromStdString(item->name());
		case 1: return QString::fromStdString(item->host());
		case 2: return QString::fromStdString(item->port());
		default: return QVariant();
		}
	}
	else if(role == Qt::CheckStateRole)
	{
		if(index.column() == 0)
		{
			return data_->selected(index.row());
		}
	}

	return QVariant();
}

bool ServerListModel::setData(const QModelIndex& index, const QVariant&  value, int role)
{
	if(!index.isValid())
	{
		return false;
	}

	if(index.column() == 0)
	{
		if(role == Qt::CheckStateRole)
		{
			bool checked=(value.toInt() == Qt::Checked)?true:false;

			if(ServerItem* item=ServerList::Instance()->item(index.row()))
			{
				if(checked)
				{
					selection_.push_back(item->clone());
				}
				else
				{
					for(std::vector<ServerItem*>::iterator it=selection_.begin(); it != selection_.end(); it++)
					{
						if((*it)->match(item))
						{
							selection_.erase(it);
							delete *it;
							break;
						}
					}
				}
			}

			emit  dataChanged(index,index);

			return true;

		}
	}

	return false;
}

QVariant ServerListModel::headerData(int section,Qt::Orientation ori,int role) const
{
	if(ori != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section > 2)
	{
		return QVariant();
	}

    switch(section)
	{
		case 0: return tr("Name");
		case 1: return tr("Host");
		case 2: return tr("Port");
		default: return QVariant();
	}

    return QVariant();
}

QModelIndex ServerListModel::index(int row, int column, const QModelIndex& /*parent*/) const
{
	return createIndex(row,column,0);
}

QModelIndex ServerListModel::parent(const QModelIndex &) const
{
	return QModelIndex();
}

Qt::ItemFlags ServerListModel::flags( const QModelIndex &index) const
{
	//return QAbstractItemModel::flags(index);

	Qt::ItemFlags defaultFlags;


	defaultFlags=Qt::ItemIsEnabled |
		  	Qt::ItemIsSelectable |
		   	Qt::ItemIsUserCheckable | Qt::ItemIsEditable;

	return defaultFlags;
}
