//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ZombieModel.hpp"

#include "ModelColumn.hpp"

#include <QDebug>

#include "boost/date_time/posix_time/posix_time.hpp"

ZombieModel::ZombieModel(QObject *parent) :
          QAbstractItemModel(parent),
          columns_(0)
{
	columns_=ModelColumn::def("zombie_columns");

	assert(columns_);
}

ZombieModel::~ZombieModel()
= default;

void ZombieModel::setData(const std::vector<Zombie>& data)
{
	beginResetModel();
	data_=data;
	endResetModel();
}

bool ZombieModel::updateData(const std::vector<Zombie>& data)
{
	bool sameAs=false;
	if(hasData() && data.size() == data_.size())
	{
		sameAs=true;
		for(const auto & it : data)
		{
			bool hasIt=false;
			std::string p=it.path_to_task();
			for(std::vector<Zombie>::const_iterator itM=data_.begin(); itM != data_.end(); itM++)
			{
				if(p == (*itM).path_to_task())
				{
					hasIt=true;
					break;
				}
			}

			if(!hasIt)
			{
				sameAs=false;
				break;
			}
		}
	}

	if(sameAs)
	{
		data_=data;
		Q_EMIT dataChanged(index(0,0),index(data_.size()-1,columns_->count()));
		return false;
	}
	else
	{
		beginResetModel();
		data_=data;
		endResetModel();
		return true;
	}
}


void ZombieModel::clearData()
{
	beginResetModel();
	data_.clear();
	endResetModel();
}

bool ZombieModel::hasData() const
{
	return !data_.empty();
}

int ZombieModel::columnCount( const QModelIndex& /*parent */) const
{
   	 return columns_->count();
}

int ZombieModel::rowCount( const QModelIndex& parent) const
{
	if(!hasData())
		return 0;

	//Parent is the root:
	if(!parent.isValid())
	{
		return static_cast<int>(data_.size());
	}

	return 0;
}

Qt::ItemFlags ZombieModel::flags ( const QModelIndex & index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ZombieModel::data( const QModelIndex& index, int role ) const
{
	if(!index.isValid() || !hasData())
    {
		return QVariant();
	}

	int row=index.row();
    if(row < 0 || row >= static_cast<int>(data_.size()))
		return QVariant();

	QString id=columns_->id(index.column());

	if(role == Qt::DisplayRole)
	{
		if(id == "path")
            return QString::fromStdString(data_[row].path_to_task());
		else if(id == "type")
            return QString::fromStdString(data_[row].type_str());
		else if(id == "tryno")
            return data_[row].try_no();
		else if(id == "duration")
            return QString::number(data_[row].duration()) + " s";
		else if(id == "creation")
		{
            const boost::posix_time::ptime& t=  data_[row].creation_time();
			return QString::fromStdString(boost::posix_time::to_simple_string(t));
		}
		else if(id == "allowed")
            return QString::number(data_[row].allowed_age()) + " s";
		else if(id == "calls")
            return data_[row].calls();
		else if(id == "action")
            return QString::fromStdString(data_[row].user_action_str());
		else if(id == "password")
            return QString::fromStdString(data_[row].jobs_password());
		else if(id == "child")
            return QString::fromStdString(ecf::Child::to_string(data_[row].last_child_cmd()));
		else if(id == "pid")
            return QString::fromStdString(data_[row].process_or_remote_id());
		else
			return QVariant();
	}

	return QVariant();
}

QVariant ZombieModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole ))
      		  return QAbstractItemModel::headerData( section, orient, role );

	if(role == Qt::DisplayRole)
		return columns_->label(section);
	else if(role == Qt::UserRole)
		return columns_->id(section);

	return QVariant();
}

QModelIndex ZombieModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex ZombieModel::parent(const QModelIndex &child) const
{
	return {};
}

Zombie ZombieModel::indexToZombie(const QModelIndex& idx) const
{
    if(idx.isValid() && hasData())
    {
        int row=idx.row();
        if(row >= 0 || row < static_cast<int>(data_.size()))
            return data_[row];
    }
    return Zombie();
}
