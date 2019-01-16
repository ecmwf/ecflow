//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "SuiteModel.hpp"

#include "ServerHandler.hpp"
#include "SuiteFilter.hpp"
#include "VNode.hpp"
#include "UiLog.hpp"

//#define SUITEMODEL_UI_DEBUG_

SuiteModel::SuiteModel(QObject *parent) :
     QAbstractItemModel(parent),
     presentCol_(QColor(1,128,73)),
     notPresentCol_(QColor(255,0,0))
{
}

SuiteModel::~SuiteModel()
{
	beginResetModel();
	clearData();
	endResetModel();
}

void SuiteModel::clearData()
{
    server_=nullptr;

    if(data_)
		delete data_;

	data_=nullptr;

	if(realData_)
		realData_->removeObserver(this);

	realData_=nullptr;
    edited_=false;
}

void SuiteModel::setEdited(bool e)
{
    edited_=e;
}

void SuiteModel::setData(SuiteFilter* filter,ServerHandler* server)
{
	beginResetModel();

	if(data_ && data_ != filter)
	{
		clearData();
	}

	if(filter)
	{
        server_=server;
        Q_ASSERT(server);
        data_=filter->clone();
		realData_=filter;
		realData_->addObserver(this);
	}

	endResetModel();
}

void SuiteModel::notifyChange(SuiteFilter *filter)
{
    if(filter && filter == realData_)
	{
		updateData();
	}
}

void SuiteModel::notifyDelete(SuiteFilter *filter)
{
	if(filter && filter == realData_)
	{
		beginResetModel();
		clearData();
		endResetModel();
	}
}

void SuiteModel::updateData()
{
#if 0
    if(realData_ && data_ &&
         !data_->sameAs(realData_->loaded()))
        {
            beginResetModel();
            data_->setLoaded(realData_->loaded());
            endResetModel();
        }
#endif

    if(realData_ && data_ &&
         !data_->sameAs(realData_))
        {
            bool filteredChanged=false;
            beginResetModel();

            if(edited_)
                filteredChanged=data_->merge(realData_);
            else
            {
                delete data_;
                data_=realData_->clone();
            }

            endResetModel();

            if(filteredChanged)
                Q_EMIT dataUpdated();
        }
}

void SuiteModel::resetData()
{
    beginResetModel();

    if(data_)
        delete data_;

    if(realData_)
        data_=realData_->clone();

    endResetModel();
}

void SuiteModel::reloadData()
{
	beginResetModel();
	endResetModel();
}

bool SuiteModel::hasData() const
{
	return (data_ && data_->count()  > 0);
}

int SuiteModel::columnCount( const QModelIndex& /*parent */ ) const
{
     return 3;
}

int SuiteModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	//Parent is the root:
	if(!parent.isValid())
	{
		return data_->count();
	}

	return 0;
}

Qt::ItemFlags SuiteModel::flags ( const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags;

	if(data_->isEnabled())
	{
		defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable; // | Qt::ItemIsEditable;
	}

	return defaultFlags;
}

QVariant SuiteModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid() || !hasData())
    {
		return QVariant();
	}

	//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	//if(role != Qt::DisplayRole && role != Qt::BackgroundRole && role != Qt::ForegroundRole)
	//{
	//	return QVariant();
	//}

	int row=index.row();
	if(row < 0 || row >= data_->count())
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0:
            return QString::fromStdString(data_->items().at(row).name());
			break;
		case 1:
            return (data_->items().at(row).loaded())?"loaded":"not loaded";
			break;
        case 2:
            {
                if(data_->items().at(row).loaded())
                {
                    Q_ASSERT(server_);
                    int n=server_->vRoot()->totalNumOfTopLevel(data_->items().at(row).name());
                    if(n != -1)
                        return n;
                    else
                        return QVariant();
                }
                else
                    return QVariant();
            }
            break;
        default:
			break;
		}
	}
	else if(role == Qt::CheckStateRole)
	{
		if(index.column()==0)
            return (data_->items().at(row).filtered())?QVariant(Qt::Checked):QVariant(Qt::Unchecked);
	}
	else if(role == Qt::ForegroundRole)
	{
		if(!data_->isEnabled())
		{
			return QVariant();
		}
		else if(index.column() == 1)
		{
            return (data_->items().at(row).loaded())?presentCol_:notPresentCol_;
		}
		return QVariant();
	}

	return QVariant();
}

bool SuiteModel::setData(const QModelIndex& index, const QVariant & value, int role )
{
	if(index.column() == 0 && role == Qt::CheckStateRole)
	{
		int row=index.row();
		if(row >=0 && row < data_->count())
		{
			bool checked=(value.toInt() == Qt::Checked)?true:false;
			data_->setFiltered(row,checked);			
            edited_=true;
            Q_EMIT dataChanged(index,index);
			return true;
		}
	}

	return false;
}

QVariant SuiteModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
      		  return QAbstractItemModel::headerData( section, orient, role );

   	if(role == Qt::DisplayRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Suite");
        case 1: return tr("Load status on server");
        case 2: return tr("Number of children");
   		default: return QVariant();
   		}
   	}
   	else if(role== Qt::ToolTipRole)
   	{
   		switch ( section )
   		{
        case 0: return tr("Filter status of suite");
        case 1: return tr("Indicates if the suite is currently <b>loaded</b> on the server.<br><br>\
                          Please note: this information might not be up to date for <b>unfiltered</b> suites. Use <u>Fetch load status</u> \
                          to get the actual load status list.");
        case 2: return tr("Number of children");
   		default: return QVariant();
   		}
   	}
    return QVariant();
}

QModelIndex SuiteModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return {};
	}

	//When parent is the root this index refers to a node or server
	if(!parent.isValid())
	{
		return createIndex(row,column);
	}

	return QModelIndex();

}

QModelIndex SuiteModel::parent(const QModelIndex &child) const
{
	return {};
}
