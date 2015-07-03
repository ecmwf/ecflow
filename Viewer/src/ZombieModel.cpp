//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ZombieModel.hpp"

#include <QDebug>

ZombieModel::ZombieModel(QObject *parent) :
          QAbstractItemModel(parent)
{

}

ZombieModel::~ZombieModel()
{
}

void ZombieModel::setData(const std::vector<Zombie>& data)
{
	beginResetModel();
	data_=data;
	endResetModel();
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

int ZombieModel::columnCount( const QModelIndex& /*parent */ ) const
{
   	 return 11;
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


	/*
	ecf::Child::ZombieType type() const { return zombie_type_;}
		ecf::Child::CmdType last_child_cmd() const { return last_child_cmd_; }
		const ZombieAttr& attr() const { return attr_;}
		int calls() const { return calls_;}

		std::string type_str() const;
		const std::string& jobs_password() const { return jobs_password_; }
		const std::string& path_to_task() const { return path_to_task_; }
		const std::string& process_or_remote_id() const { return process_or_remote_id_; }
	 	int try_no() const { return try_no_; }
		int duration() const { return duration_; }
		ecf::User::Action user_action() const;
		std::string user_action_str() const;


		const boost::posix_time::ptime&  creation_time() const { return creation_time_; }

		bool empty() const { return path_to_task_.empty(); }

		/// returns in seconds the age the zombie is allowed to live
		/// Server typically checks every 60 seconds, hence this is lowest valid value that has effect
		int allowed_age() const;

		*/

	int row=index.row();
	if(row < 0 || row >= data_.size())
		return QVariant();

	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
		case 0:
			return QString::fromStdString(data_.at(row).path_to_task());
			break;
		case 1:
			return QString::fromStdString(data_.at(row).type_str());
			break;
		case 2:
			return data_.at(row).try_no();
			break;
		case 3:
			return data_.at(row).duration();
			break;
		case 4:
			{
				//const boost::posix_time::ptime& =  data_.at(row).creation_time();
				return QVariant();
			}
			break;

		case 5:
			return QString::number(data_.at(row).allowed_age()) + " s";
			break;
		case 6:
			return data_.at(row).calls();
			break;

		case 7:
			return QString::fromStdString(data_.at(row).user_action_str());
			break;
		case 8:
			return QString::fromStdString(data_.at(row).jobs_password());
			break;

		case 9:
			return data_.at(row).last_child_cmd();
			break;

		case 10:
			return QString::fromStdString(data_.at(row).process_or_remote_id());
			break;

		default:
			break;
		}
	}

	return QVariant();
}

QVariant ZombieModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
	if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
      		  return QAbstractItemModel::headerData( section, orient, role );

   	if(role == Qt::DisplayRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Path");
   		case 1: return tr("Type");
   		case 2: return tr("Try no");
   		case 3: return tr("Duration");
   		case 4: return tr("Creation");
   		case 5: return tr("Allowed age");
   		case 6: return tr("Calls");
   		case 7: return tr("User action");
   		case 8: return tr("Password");
   		case 9: return tr("Child cmd");
   		case 10: return tr("Id");
   		default: return QVariant();
   		}
   	}
   	else if(role== Qt::ToolTipRole)
   	{
   		switch ( section )
   		{
   		case 0: return tr("Path to the task node");
   		case 1: return tr("Type of the zombie");
   		case 2: return tr("Try number");
   		case 3: return tr("Duration");
   		case 4: return tr("Creation time");
   		case 5: return tr("The age the zombie is allowed to live. Server typically checks every 60 seconds, hence this is lowest valid value that has effect");
   		case 6: return tr("Number of calls");
   		case 7: return tr("User action");
   		case 8: return tr("Password of the job");
   		case 9: return tr("Last child command");
   		case 10: return tr("Process or remote id");
   		default: return QVariant();
   		}
   	}
    return QVariant();
}

QModelIndex ZombieModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex ZombieModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}
