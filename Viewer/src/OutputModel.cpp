//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include <QDir>
#include <QFont>

QColor OutputModel::joboutCol_=QColor(0,115,48);

//=======================================================================
//
// OutputModel
//
//=======================================================================

OutputModel::OutputModel(QObject *parent) :
          QAbstractItemModel(parent),
          joboutRow_(-1)

{
}

#if 0
void OutputModel::setData(VDir_ptr dir,const std::string& jobout)
{
	beginResetModel();
	dir_=dir;
    joboutRow_=-1;

    if(dir_)
    {
        for(int i=0; i < dir_->count(); i++)
        {
            if(dir_->fullName(i) == jobout)
            {
                joboutRow_=i;
                break;
            }
        }
    }

	endResetModel();
}
#endif

void OutputModel::setData(const std::vector<VDir_ptr>& dirs,const std::string& jobout)
{
    beginResetModel();
    dirs_=dirs;
    joboutRow_=-1;

    for(std::size_t i=0; i < dirs_.size(); i++)
    {
        if(dirs_[i])
        {
            int idx=dirs_[i]->findByFullName(jobout);
            if(idx != -1)
            {
                joboutRow_=idx;
                break;
            }
        }
     }

    endResetModel();
}

void OutputModel::clearData()
{
	beginResetModel();
    dirs_.clear();
	endResetModel();
}

int OutputModel::columnCount( const QModelIndex& parent  ) const
{
    return 5;
}

int OutputModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	if(!parent.isValid())
    {
        int cnt=0;
        for(std::size_t i=0; i < dirs_.size(); i++)
        {
            if(dirs_[i])
                cnt+=dirs_[i]->count();
        }
        return cnt;
    }

	return 0;
}

QVariant  OutputModel::data(const QModelIndex& index, int role) const
{
    if(!hasData() || (role != Qt::DisplayRole && role != Qt::UserRole &&
        role != Qt::ForegroundRole && role != Qt::ToolTipRole && role != Qt::FontRole))
		return QVariant();

	int row=index.row();
    VDir_ptr dir;
    VDirItem *item=itemAt(row,dir);

    if(!item || !dir)
        return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0:
            return QString::fromStdString(dir->path() + "/" + item->name_);
		case 1:
			return formatSize(item->size_);
		case 2:
			return formatAgo(item->mtime_);
		case 3:
            return formatDate(item->mtime_);
        case 4:
            return QString::fromStdString(dir->fetchModeStr());
		default:
			break;
		}
	}
	else if(role == Qt::UserRole)
	{
		switch(index.column())
		{
		case 0:
			return QString::fromStdString(fullName(index));
		case 1:
			return static_cast<float>(item->size_)/1024.;
		case 2:
			return item->mtime_.toTime_t();
		default:
			break;
		}
	}
    else if(role == Qt::ForegroundRole)
    {
        if(row == joboutRow_)
        {
            return joboutCol_;
        }
    }
    else if(role == Qt::FontRole)
    {
        if(row == joboutRow_)
        {
            QFont f;
            f.setBold(true);
            return f;
        }
    }
    else if(role == Qt::ToolTipRole)
    {
        if(row == joboutRow_)
        {
            return QString::fromStdString(item->name_) + " is the current job output file.";
        }
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
    case 2: return tr("Modified (ago)");
    case 3: return tr("Modified");
    case 4: return tr("Source");
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
		return createIndex(row,column,static_cast<void*>(0));
	}

	return QModelIndex();

}

QModelIndex OutputModel::parent(const QModelIndex &child) const
{
	return QModelIndex();

}

VDirItem* OutputModel::itemAt(int row,VDir_ptr& dir) const
{
    for(std::size_t i=0; i < dirs_.size(); i++)
    {
        if(dirs_[i])
        {
            int cnt=dirs_[i]->count();
            if(row < cnt)
            {
                Q_ASSERT(row>=0);
                dir=dirs_[i];
                return dirs_[i]->items()[row];
            }

            row-=cnt;
       }
    }

    return 0;
}

bool OutputModel::hasData() const
{
    for(std::size_t i=0; i < dirs_.size(); i++)
        if(dirs_[i])
            return true;

    return false;
}

std::string OutputModel::fullName(const QModelIndex& index) const
{
	if(!hasData())
		return std::string();

    int row=index.row();
    for(std::size_t i=0; i < dirs_.size(); i++)
    {
        if(dirs_[i])
        {
            int cnt=dirs_[i]->count();
            if(row < cnt)
            {
                Q_ASSERT(row >=0);
                return dirs_[i]->fullName(row);
            }
            row-=cnt;
       }
    }

    return std::string();
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

QString OutputModel::formatDate(QDateTime dt) const
{
  	//QDateTime dt=QDateTime::fromTime_t(t);
	return dt.toString("yyyy-MM-dd hh:mm:ss");
}

QString OutputModel::formatAgo(QDateTime dt) const
{
	QString str=tr("Right now");

	QDateTime now=QDateTime::currentDateTime();

	int delta  = dt.secsTo(now);
	if(delta<0) delta = 0;

	if(delta ==1)
		str=tr("1 second ago");

	else if(delta >=1  && delta < 60)
	{
		str=QString::number(delta) + tr(" second") +  ((delta==1)?tr(""):tr("s")) +  tr(" ago");
	}

	else if(delta >= 60 && delta < 60*60)
	{
		int val=delta/60;
		str=QString::number(val) + tr(" minute") +  ((val==1)?tr(""):tr("s")) +  tr(" ago");
	}

	else if(delta >= 60*60 && delta < 60*60*24)
	{
		int val=delta/(60*60);
		str=QString::number(val) + tr(" hour") +  ((val==1)?tr(""):tr("s")) +  tr(" ago");
	}

	else if(delta >= 60*60*24)
	{
		int val=delta/(60*60*24);
		str=QString::number(val) + tr(" day") +  ((val==1)?tr(""):tr("s")) +  tr(" ago");
	}

	return str;
}

//=======================================================================
//
// OutputSortModel
//
//=======================================================================

OutputSortModel::OutputSortModel(QObject* parent) :
	QSortFilterProxyModel(parent)
{

}

bool OutputSortModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
	int col=sourceLeft.column();

	//Name or modification date
	if(col==0 || col == 3)
	{
		return sourceModel()->data(sourceLeft,Qt::DisplayRole).toString() < sourceModel()->data(sourceRight,Qt::DisplayRole).toString();
	}
	//Size
	else if(col == 1)
	{
		return sourceModel()->data(sourceLeft,Qt::UserRole).toFloat() < sourceModel()->data(sourceRight,Qt::UserRole).toFloat();
	}
	//Ago
	else if(col == 2)
	{
		return sourceModel()->data(sourceLeft,Qt::UserRole).toInt() > sourceModel()->data(sourceRight,Qt::UserRole).toInt();
	}

	return true;
}

bool OutputSortModel::filterAcceptsRow(int sourceRow,const QModelIndex& sourceParent) const
{
	return true;
}

QModelIndex OutputSortModel::fullNameToIndex(const std::string& fullName)
{
	QString name=QString::fromStdString(fullName);

	for(int i=0; i < rowCount(QModelIndex()); i++)
	{
		QModelIndex idx=index(i,0);
		if(name.endsWith(data(idx,Qt::DisplayRole).toString()))
		{
			return idx;
		}
	}
	return QModelIndex();
}

//========================================================
//
// OutputDirLitsDelegate
//
//========================================================

OutputDirLitsDelegate::OutputDirLitsDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
}

void OutputDirLitsDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
    if(index.column()==0)
    {
        QStyleOptionViewItem vopt(option);
        initStyleOption(&vopt, index);
        vopt.textElideMode=Qt::ElideLeft;
        QStyledItemDelegate::paint(painter,vopt,index);
    }
    else
    {
        QStyledItemDelegate::paint(painter,option,index);
    }
}

