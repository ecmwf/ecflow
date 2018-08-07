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
          QAbstractItemModel(parent)
{
}

void OutputModel::setData(const std::vector<VDir_ptr>& dirs,const std::string& jobout)
{
    beginResetModel();
    dirs_=dirs;
    joboutRows_.clear();

    for(auto & dir : dirs_)
    {
        if(dir)
        {
            int idx=dir->findByFullName(jobout);
            if(idx != -1)
            {
                joboutRows_.insert(idx);
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
    return 6;
}

int OutputModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	if(!parent.isValid())
    {
        int cnt=0;
        for(const auto & dir : dirs_)
        {
            if(dir)
                cnt+=dir->count();
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
            return QString::fromStdString(item->name_);
        case 1:
            return QString::fromStdString(dir->path());
        case 2:
			return formatSize(item->size_);
        case 3:
			return formatAgo(item->mtime_);
        case 4:
            return formatDate(item->mtime_);
        case 5:
            return QString::fromStdString(dir->fetchModeStr());
		default:
			break;
		}
    }
    else if(role == Qt::ForegroundRole)
    {
        if(joboutRows_.find(row) != joboutRows_.end())
        {
            return joboutCol_;
        }
    }
    else if(role == Qt::FontRole)
    {
        if(joboutRows_.find(row) != joboutRows_.end())
        {
            QFont f;
            f.setBold(true);
            return f;
        }
    }
    else if(role == Qt::ToolTipRole)
    {
        if(joboutRows_.find(row) != joboutRows_.end())
        {
            return QString::fromStdString(item->name_) + " is the current job output file.";
        }
    }
    //Used for sorting
    else if(role == Qt::UserRole)
    {
        switch(index.column())
        {
        case 0:
            return QString::fromStdString(item->name_);
        case 1:
            return QString::fromStdString(dir->path());
        case 2:
            return item->size_;
        case 3:
            return secsToNow(item->mtime_);
        case 4:
            return item->mtime_.toTime_t();
        case 5:
            return QString::fromStdString(dir->fetchModeStr());
        default:
            break;
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
    case 1: return tr("Path");
    case 2: return tr("Size");
    case 3: return tr("Modified (ago)");
    case 4: return tr("Modified");
    case 5: return tr("Source");
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
    for(const auto & i : dirs_)
    {
        if(i)
        {
            int cnt=i->count();
            if(row < cnt)
            {
                Q_ASSERT(row>=0);
                dir=i;
                return i->items()[row];
            }

            row-=cnt;
       }
    }

    return 0;
}

bool OutputModel::hasData() const
{
    for(const auto & dir : dirs_)
        if(dir)
            return true;

    return false;
}

std::string OutputModel::fullName(const QModelIndex& index) const
{
	if(!hasData())
		return std::string();

    int row=index.row();
    for(const auto & dir : dirs_)
    {
        if(dir)
        {
            int cnt=dir->count();
            if(row < cnt)
            {
                Q_ASSERT(row >=0);
                return dir->fullName(row);
            }
            row-=cnt;
       }
    }

    return std::string();
}

void OutputModel::itemDesc(const QModelIndex& index,std::string& itemFullName,VDir::FetchMode& mode) const
{
    itemFullName.clear();

    if(!hasData())
        return;

    int row=index.row();
    for(const auto & dir : dirs_)
    {
        if(dir)
        {
            int cnt=dir->count();
            if(row < cnt)
            {
                Q_ASSERT(row >=0);
                itemFullName=dir->fullName(row);
                mode=dir->fetchMode();
                return;
            }
            row-=cnt;
       }
    }
}

QModelIndex OutputModel::itemToIndex(const std::string& itemFullName,VDir::FetchMode fetchMode) const
{
    int row=0;;
    for(const auto & dir : dirs_)
    {
        if(dir)
        {
            if(dir->fetchMode() == fetchMode)
            {
                for(int j=0; j < dir->count(); j++, row++)
                    if(dir->fullName(j) == itemFullName)
                        return index(row,0);
            }
            else
                row+=dir->count();
        }
    }
    return QModelIndex();
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

qint64 OutputModel::secsToNow(QDateTime dt) const
{
    QDateTime now=QDateTime::currentDateTime();

    qint64 delta  = dt.secsTo(now);
    return (delta<0)?0:delta;
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
    if(index.column()==1)
    {
        QStyleOptionViewItem vopt(option);
        initStyleOption(&vopt, index);
        vopt.textElideMode=Qt::ElideRight;
        QStyledItemDelegate::paint(painter,vopt,index);
    }
    else
    {
        QStyledItemDelegate::paint(painter,option,index);
    }
}

