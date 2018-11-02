//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TimelineInfoWidget.hpp"

#include <QtGlobal>
#include <QFileInfo>
#include <QToolButton>
#include <QVBoxLayout>

#include "TimelineData.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "VNState.hpp"

#include "ui_TimelineInfoWidget.h"

//=======================================================
//
// TimelineInfoDialog
//
//=======================================================

TimelineInfoDialog::TimelineInfoDialog(QWidget* parent) : QDialog(parent)
{
        QVBoxLayout *vb=new QVBoxLayout(this);
        infoW_=new TimelineInfoWidget(this);
        vb->addWidget(infoW_);

        QToolButton *closeTb=new QToolButton(this);
        closeTb->setText("Close");
        vb->addWidget(closeTb);

        connect(closeTb,SIGNAL(clicked()),
                this,SLOT(reject()));
}


//=======================================================
//
// TimelineInfoModel
//
//=======================================================

TimelineInfoModel::TimelineInfoModel(QObject *parent) :
          QAbstractItemModel(parent), data_(0)
{
}

TimelineInfoModel::~TimelineInfoModel()
{
}

void TimelineInfoModel::setData(TimelineItem *data)
{
    beginResetModel();
    data_=data;
    endResetModel();
}

void TimelineInfoModel::clearData()
{
    beginResetModel();
    data_=0;
    endResetModel();
}

bool TimelineInfoModel::hasData() const
{
    return (data_ != NULL);
}

int TimelineInfoModel::columnCount( const QModelIndex& /*parent */) const
{
     return 3;
}

int TimelineInfoModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return static_cast<int>(data_->size());
    }

    return 0;
}

Qt::ItemFlags TimelineInfoModel::flags ( const QModelIndex & index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TimelineInfoModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }

    int row=index.row();
    if(row < 0 || row >= static_cast<int>(data_->size()))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        if(index.column() == 0)
        {
            return TimelineItem::toQDateTime(data_->start_[row]).toString("hh:mm:ss dd/MM/yyyy");
        }
        else if(index.column() == 1)
        {
            //UiLog().dbg() << "xp=" << xp << " time=" << data->items()[row].start_[i];
            if(VNState* vn=VNState::find(data_->status_[row]))
            {
                return vn->name();
            }
        }
        else
            return QVariant();
    }

    if(role == Qt::BackgroundRole)
    {
       if(index.column() == 1)
       {
            //UiLog().dbg() << "xp=" << xp << " time=" << data->items()[row].start_[i];
            if(VNState* vn=VNState::find(data_->status_[row]))
            {
                return vn->colour();
            }
       }
    }
    return QVariant();
}

QVariant TimelineInfoModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole ))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
    {
        switch(section)
        {
        case 0:
            return "When";
        case 1:
            return "Became";
        case 2:
            return "Time";
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QModelIndex TimelineInfoModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex TimelineInfoModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}


//=======================================================
//
// TimelineInfoWidget
//
//=======================================================

TimelineInfoWidget::TimelineInfoWidget(QWidget *parent) :
    ui_(new Ui::TimelineInfoWidget),
    numOfRows_(0)
{
    ui_->setupUi(this);

    //message label
    //ui_->messageLabel->hide();

    //the models
    model_=new TimelineInfoModel(this);

    ui_->timeTree->setModel(model_);
}

void TimelineInfoWidget::load(QString host, QString port,const TimelineItem& data)
{
    host_=host;
    port_=port;
    data_=data;

    model_->setData(&data_);
}
