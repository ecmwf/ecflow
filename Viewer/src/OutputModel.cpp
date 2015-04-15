//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputModel.hpp"

#include <QColor>
#include <QDebug>

#include "ServerHandler.hpp"

//=======================================================================
//
// VariabletModel
//
//=======================================================================

OutputModel::OutputModel(OutputData* data,QObject *parent) :
          QAbstractItemModel(parent),
          data_(data)
{

}

bool OutputModel::hasData() const
{
	return true; //(data_->count()  > 0);
}

int OutputModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 2;
}

int OutputModel::rowCount( const QModelIndex& parent) const
{

	/*//Parent is the root: the item must be a node or a server
	if(!parent.isValid())
	{
		return data_->count();
	}
	//The parent is a server or a node
	else if(!isVariable(parent))
	{
		int row=parent.row();
		return data_->varNum(row);
	}*/

	//parent is a variable
	return 0;
}

Qt::ItemFlags OutputModel::flags ( const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags;

	defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	return defaultFlags;
}

QVariant OutputModel::data( const QModelIndex& index, int role ) const
{
	if( !index.isValid())
    {
		return QVariant();
	}

	/*//Data lookup can be costly so we immediately return a default value for all
	//the cases where the default should be used.
	if(role != Qt::DisplayRole && role != Qt::BackgroundRole && role != Qt::ForegroundRole)
	{
		return QVariant();
	}
	//qDebug() << "data" << index << role;

	int row=index.row();
	int level=indexToLevel(index);

	//Server or node
	if(level == 1)
	{
		if(role == Qt:: BackgroundRole)
            return QColor(122,122,122);

        if(role == Qt::ForegroundRole)
            return QColor(255,255,255);

        OutputModelData *d=data_->data(row);
		if(!d)
		{
			return QVariant();
		}

		if(index.column() == 0)
		{
			if(role == Qt::DisplayRole)
			{
				return QString::fromStdString(d->name());
			}
		}

		return QVariant();
	}

	//Variables
	else if (level == 2)
	{
        OutputModelData *d=data_->data(index.parent().row());
		if(!d)
		{
			return QVariant();
		}

		//Generated variable
		if(d->isGenVar(row))
		{
			if(role == Qt::ForegroundRole)
					return QColor(70,70,70);
		}
        if(role == Qt::DisplayRole)
        {
		    if(index.column() == 0)
		    {
			    return QString::fromStdString(d->name(row));
		    }
		    else if(index.column() == 1)
		    {
			    return QString::fromStdString(d->value(row));
		    }
        }

		return QVariant();
	}*/

	return QVariant();
}


QVariant OutputModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || role != Qt::DisplayRole )
      		  return QAbstractItemModel::headerData( section, orient, role );

   	switch ( section )
	{
   	case 0: return tr("Name");
   	case 1: return tr("Modified");
   	default: return QVariant();
   	}

    return QVariant();
}

QModelIndex OutputModel::index( int row, int column, const QModelIndex & parent ) const
{
	if(!hasData() || row < 0 || column < 0)
	{
		return QModelIndex();
	}

	return createIndex(row,column);
}

QModelIndex OutputModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}
