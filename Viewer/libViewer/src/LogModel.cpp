//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LogModel.hpp"

#include <QDebug>
#include <QStringList>

#include "IconProvider.hpp"
#include "UiLog.hpp"

LogModel::LogModel(QObject *parent) :
          QAbstractItemModel(parent),
          filterPeriod_(false), periodStart_(0), periodEnd_(0),
          highlightPeriod_(false), highlightStart_(0), highlightEnd_(0)
{
	IconProvider::add(":/viewer/log_error.svg","log_error");
	IconProvider::add(":/viewer/log_info.svg","log_info");
	IconProvider::add(":/viewer/log_warning.svg","log_warning");
}

LogModel::~LogModel()
{
}

void LogModel::loadFromFile(const std::string& fileName)
{
    beginResetModel();

    filterPeriod_=false;
    periodStart_=0;
    periodEnd_=0;
    highlightPeriod_=false;
    highlightStart_=0;
    highlightEnd_=0;

    data_.loadFromFile(fileName);
    endResetModel();
}

void LogModel::setData(const std::string& data)
{
	beginResetModel();

    filterPeriod_=false;
    periodStart_=0;
    periodEnd_=0;
    highlightPeriod_=false;
    highlightStart_=0;
    highlightEnd_=0;

    data_.loadFromText(data);
#if 0
	data_.clear();

	QString in=QString::fromStdString(data);
	Q_FOREACH(QString s,in.split("\n"))
	{
		if(!s.simplified().isEmpty())
            data_.data_.push_back(LogDataItem(s.toStdString()));
	}
#endif
	endResetModel();
}

void LogModel::setData(const std::vector<std::string>& data)
{
	beginResetModel();

    filterPeriod_=false;
    periodStart_=0;
    periodEnd_=0;
    highlightPeriod_=false;
    highlightStart_=0;
    highlightEnd_=0;
    data_.loadFromText(data);
#if 0
    data_.clear();

    for(std::vector<std::string>::const_iterator it=data.begin(); it != data.end(); ++it)
	{
		QString s=QString::fromStdString(*it);
		if(!s.simplified().isEmpty())
		{
			data_ << LogModelLine(s);
		}
	}
#endif
	endResetModel();
}

void LogModel::appendData(const std::vector<std::string>& data)
{
	int num=0;

    for(std::vector<std::string>::const_iterator it=data.begin(); it != data.end(); ++it)
	{
		QString s=QString::fromStdString(*it);
		if(!s.simplified().isEmpty())
		{
			num++;
		}
	}

	if(num >0)
	{
		beginInsertRows(QModelIndex(),rowCount(),rowCount()+num-1);
        data_.appendFromText(data);
#if 0
        for(std::vector<std::string>::const_iterator it=data.begin(); it != data.end(); ++it)
		{
			QString s=QString::fromStdString(*it);
			if(!s.simplified().isEmpty())
			{
				data_ << LogModelLine(s);
			}
		}
#endif

		endInsertRows();
	}
}



void LogModel::clearData()
{
	beginResetModel();
    filterPeriod_=false;
    periodStart_=0;
    periodEnd_=0;
    highlightPeriod_=false;
    highlightStart_=0;
    highlightEnd_=0;
	data_.clear();
	endResetModel();
}

bool LogModel::hasData() const
{
    return !data_.isEmpty();
}

int LogModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 3;
}

int LogModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	//Parent is the root:
	if(!parent.isValid())
	{
        if(!filterPeriod_)
            return data_.size();
        else
            return periodEnd_-periodStart_+1;
	}

	return 0;
}

Qt::ItemFlags LogModel::flags ( const QModelIndex & index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant LogModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid() || !hasData())
    {
		return QVariant();
	}
	int row=index.row();

    if(filterPeriod_)
        row+=periodStart_;

    if(row < 0 || row >= static_cast<int>(data_.size()))
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0:
			{
                switch(data_.type(row))
				{
                case LogDataItem::MessageType:
					return "MSG ";
                case LogDataItem::LogType:
					return "LOG ";
                case LogDataItem::ErrorType:
					return "ERR ";
                case LogDataItem::WarningType:
					return "WAR ";
                case LogDataItem::DebugType:
					return "DBG ";
				default:
					return QVariant();
				}
			}
			break;
		case 1:
            return data_.date(row).toString("hh:mm:ss dd.M.yyyy");;
			break;
		case 2:
            return data_.entry(row);
			break;
		default:
			break;
		}
	}

	else if(role == Qt::DecorationRole)
	{
		if(index.column() ==0)
		{
            switch(data_.type(row))
			{
                case LogDataItem::MessageType:
					return IconProvider::pixmap("log_info",12);
                case LogDataItem::LogType:
					return IconProvider::pixmap("log_info",12);
                case LogDataItem::ErrorType:
					return IconProvider::pixmap("log_error",12);
                case LogDataItem::WarningType:
					return IconProvider::pixmap("log_warning",12);
				default:
					return QVariant();
			}
		}
	}

    else if(role == Qt::BackgroundRole)
    {
        if(highlightPeriod_)
        {
            if(static_cast<size_t>(row) >= highlightStart_ && static_cast<size_t>(row)  <= highlightEnd_)
                //return QColor(168,226,145);
                return QColor(198,223,188);
            else
                return QVariant();
        }
    }

	else if(role == Qt::FontRole)
	{
		QFont f;

        /*if(data_.at(row).type_ == LogDataItem::ErrorType)
		{
			f.setBold(true);
		}*/

		return f;
	}

    else if(role == Qt::ToolTipRole)
    {
        QString txt="<b>Type: </b>";
        switch(data_.type(row))
        {
            case LogDataItem::MessageType:
                    txt +="MSG ";
                    break;
            case LogDataItem::LogType:
                    txt +="LOG ";
                    break;
            case LogDataItem::ErrorType:
                    txt +="ERR ";
                    break;
            case LogDataItem::WarningType:
                    txt +="WAR ";
                    break;
            case LogDataItem::DebugType:
                    txt +="DBG ";
            default:
                    break;
        }

        txt+="<br><b>Date: </b>" + data_.date(row).toString("hh:mm:ss dd.M.yyyy");
        txt+="<br><b>Entry: </b>" + data_.entry(row);
        return txt;
    }

	/*else if(role == Qt::SizeHintRole)
	{
		QFont f;
		f.setBold(true);
		QFontMetrics fm(f);
		return fm.height()+10;
	}*/


	return QVariant();
}

QVariant LogModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
      		  return QAbstractItemModel::headerData( section, orient, role );

   	if(role == Qt::DisplayRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Type");
   		case 1: return tr("Date");
   		case 2: return tr("Entry");
   		default: return QVariant();
   		}
   	}
   	else if(role== Qt::ToolTipRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Type");
   		case 1: return tr("Date");
   		case 2: return tr("Entry");
   		default: return QVariant();
   		}
   	}
    return QVariant();
}

QModelIndex LogModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex LogModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

QModelIndex LogModel::lastIndex() const
{
	return index(rowCount()-1,0);
}

QString LogModel::entryText(const QModelIndex &idx) const
{
    return data(index(idx.row(),2)).toString();
}

QString LogModel::fullText(const QModelIndex &idx) const
{
    return data(index(idx.row(),0)).toString() + " " +
           data(index(idx.row(),1)).toString() + " " +
           data(index(idx.row(),2)).toString();
}

void LogModel::setPeriod(qint64 start,qint64 end)
{
    beginResetModel();
    filterPeriod_=data_.indexOfPeriod(start,end,periodStart_,periodEnd_);
    endResetModel();
}

void LogModel::setHighlightPeriod(qint64 start,qint64 end)
{
    beginResetModel();
    if(data_.indexOfPeriod(start,end,highlightStart_,highlightEnd_))
    {
        highlightPeriod_=true;
        //need a repaint
    }
    endResetModel();
}

int LogModel::realRow(size_t idx) const
{
    if(filterPeriod_)
    {
        return idx-periodStart_;
    }
    return idx;
}

QModelIndex LogModel::highlightPeriodIndex() const
{
    if(highlightPeriod_)
    {
        return index(realRow(highlightStart_),0);
    }
    return QModelIndex();
}

//========================================================
//
// LogDelegate
//
//========================================================

LogDelegate::LogDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{

}

void LogDelegate::paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const
{
	QStyledItemDelegate::paint(painter,option,index);

	/*if(index.column()==11)
	{
        QStyleOptionViewItem vopt(option);
		initStyleOption(&vopt, index);

		const QStyle *style = vopt.widget ? vopt.widget->style() : QApplication::style();
		const QWidget* widget = vopt.widget;

		QString text=index.data(Qt::DisplayRole).toString();
		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);
		if(text == "ERR")
		{
			QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &vopt, widget);
		}

		painter->fillRect(textRect,Qt::red);
		painter->drawText(textRect,Qt::AlignLeft | Qt::AlignVCenter,text);
	}
	else
	{
		QStyledItemDelegate::paint(painter,option,index);
	}*/
}


QSize LogDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	QSize size=QStyledItemDelegate::sizeHint(option,index);

	size+=QSize(0,2);

    return size;
}
