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
#include <QDateTime>
#include <QDebug>

//=======================================================================
//
// OutputModel
//
//=======================================================================

OutputModel::OutputModel(QObject *parent) :
          QAbstractItemModel(parent)

{
	//setIconProvider(0);
}

void OutputModel::setData(VDir_ptr dir)
{
	beginResetModel();
	dir_=dir;
	endResetModel();
}

int OutputModel::columnCount( const QModelIndex& parent  ) const
{
	return 3;
}

int OutputModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	if(!parent.isValid())
		return dir_->count();

	return 0;
}

QVariant  OutputModel::data(const QModelIndex& index, int role) const
{
	if(!hasData() || role != Qt::DisplayRole)
		return QVariant();

	int row=index.row();
	VDirItem *item=dir_->items().at(row);

	switch(index.column())
	{
	case 0:
		return QString::fromStdString(item->name_);
	case 1:
		return formatSize(item->size_);
	case 2:
		return formatDate(item->mtime_);
	default:
		break;
	}

	return QVariant();
}

QVariant OutputModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || role != Qt::DisplayRole )
      		  return QAbstractItemModel::headerData( section, orient, role );

   	switch ( section )
	{
   	case 0: return tr("Name");
   	case 1: return tr("Size");
   	case 2: return tr("Modified");
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

	//When parent is the root this index refers to a node or server
	if(!parent.isValid())
	{
		return createIndex(row,column,0);
	}

	return QModelIndex();

}

QModelIndex OutputModel::parent(const QModelIndex &child) const
{
	return QModelIndex();

}

bool OutputModel::hasData() const
{
	return dir_ && dir_.get();
}

QString OutputModel::formatSize(unsigned int size) const
{
  	if(size < 1024)
	  	return QString::number(size) + " B";
	else if(size < 1024*1024)
	  	return QString::number(size/1024) + " KB";
	else if(size < 1024*1024*1024)
	  	return QString::number(size/(1024*1024)) + " MB";
	else
	  	return QString::number(size/(1024*1024*1024)) + " GB";

 	return QString();
}

QString OutputModel::formatDate(const std::time_t& t) const
{
  	QDateTime dt=QDateTime::fromTime_t(t);
	return dt.toString("yyyy-MM-dd hh:mm");
}
