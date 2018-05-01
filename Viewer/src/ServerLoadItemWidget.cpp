//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerLoadItemWidget.hpp"

#include <QDebug>
#include <QHBoxLayout>
#include <QSortFilterProxyModel>

#include "Node.hpp"

#include "InfoProvider.hpp"
#include "LogLoadWidget.hpp"
#include "ServerHandler.hpp"
//#include "ServerLoadView.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"
#include "VNState.hpp"


ServerLoadItemWidget::ServerLoadItemWidget(QWidget *parent)
{
    //ui_->setupUi(this);

    QHBoxLayout* hb=new QHBoxLayout(this);
    w_=new LogLoadWidget(this);
    hb->addWidget(w_);

    //We will not keep the contents when the item becomes unselected
    unselectedFlags_.clear();

    //ui_->resCombo->addItem("seconds",0);
    //ui_->resCombo->addItem("minutes",0);

    //connect(ui_->resCombo,SIGNAL(currentIndexChanged(int)),
     //       this,SLOT(resolutionChanged(int)));

    //data_=new LogLoadData();

    //ui_->loadView->setData(data_);

#if 0
    suiteModel_=new LogLoadSuiteModel(this);
    suiteSortModel_=new QSortFilterProxyModel(this);
    suiteSortModel_->setSourceModel(suiteModel_);
    suiteSortModel_->setSortRole(Qt::UserRole);
    suiteSortModel_->setDynamicSortFilter(true);

    ui_->suiteTree->setRootIsDecorated(false);
    ui_->suiteTree->setAllColumnsShowFocus(true);
    ui_->suiteTree->setUniformRowHeights(true);
    ui_->suiteTree->setSortingEnabled(true);
    ui_->suiteTree->sortByColumn(1, Qt::DescendingOrder);
    ui_->suiteTree->setModel(suiteSortModel_);
    suiteModel_->setData(data_);

    connect(suiteModel_,SIGNAL(checkStateChanged(int,bool)),
            ui_->loadView,SLOT(addRemoveSuite(int,bool)));
#endif

    //messageLabel_->hide();
    //fileLabel_->hide();

    //Will be used for ECFLOW-901
    //infoProvider_=new WhyProvider(this);

    //QHBoxLayout* hb=new QHBoxLayout(this);

    //view_=new ServerLoadView(this);
    //hb->addWidget(view_);
}

ServerLoadItemWidget::~ServerLoadItemWidget()
{
    clearContents();
}


QWidget* ServerLoadItemWidget::realWidget()
{
    return this;
}

void ServerLoadItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    //set the info. we do not need to observe the node!!!
    info_=info;

    load();
}

void ServerLoadItemWidget::load()
{
    //textEdit_->clear();
    if(info_)
    {
        //"/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log"
        w_->load("/home/graphics/cgr/ecflow_dev/vsms1.ecf.log");
    }
}

void ServerLoadItemWidget::clearContents()
{
    InfoPanelItem::clear();
}


void ServerLoadItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //If we are here this item is active but not selected!

        //When it becomes suspended we need to clear everything since the
        //tree is probably cleared at this point
        if(suspended_)
        {
            //textEdit_->clear();
        }
        //When we leave the suspended state we need to reload everything
        else
        {
            load();
        }
    }

    Q_ASSERT(!flags.isSet(SelectedChanged));

}


//After each sync we need to reaload the contents
void ServerLoadItemWidget::serverSyncFinished()
{
    if(frozen_)
        return;

    //We do not track changes when the item is not selected
    if(!selected_ || !active_)
        return;

    if(!info_)
        return;

    //For any change we nee to reload
    load();
}

#if 0

void ServerLoadItemWidget::resolutionChanged(int)
{
    int idx=ui_->resCombo->currentIndex();
    if(idx == 0)
        ui_->loadView->setResolution(LogLoadData::SecondResolution);
    else if(idx == 1)
        ui_->loadView->setResolution(LogLoadData::MinuteResolution);
}

#endif


static InfoPanelItemMaker<ServerLoadItemWidget> maker1("server_load");


#if 0
LogLoadSuiteModel::LogLoadSuiteModel(QObject *parent) :
          QAbstractItemModel(parent), data_(0)
{
}

LogLoadSuiteModel::~LogLoadSuiteModel()
{
}

void LogLoadSuiteModel::setData(LogLoadData* data)
{
    beginResetModel();
    data_=data;
    endResetModel();
}


void LogLoadSuiteModel::clearData()
{
    beginResetModel();
    //data_.clear();
    endResetModel();
}

bool LogLoadSuiteModel::hasData() const
{
    return data_; //isEmpty();
}

int LogLoadSuiteModel::columnCount( const QModelIndex& /*parent */ ) const
{
     return 2;
}

int LogLoadSuiteModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return data_->suites().size();
    }

    return 0;
}

Qt::ItemFlags LogLoadSuiteModel::flags ( const QModelIndex & index) const
{
    Qt::ItemFlags defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if(index.column() == 0)
    {
        defaultFlags=defaultFlags | Qt::ItemIsUserCheckable;
    }
    return defaultFlags;
}

QVariant LogLoadSuiteModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }
    int row=index.row();
    if(row < 0 || row >= static_cast<int>(data_->suites().size()))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0:
            return QString::fromStdString(data_->suites()[row].name());
            break;
        case 1:
            return formatPrecentage(data_->suites()[row].percentage());
            break;
        default:
            break;
        }
    }
    else if(role == Qt::UserRole)
    {
        switch(index.column())
        {
        case 0:
            return QString::fromStdString(data_->suites()[row].name());
            break;
        case 1:
            return data_->suites()[row].percentage();
            break;
        default:
            break;
        }
    }
    return QVariant();
}

bool LogLoadSuiteModel::setData(const QModelIndex& idx, const QVariant & value, int role )
{
    if(idx.column() == 0 && role == Qt::CheckStateRole)
    {
        QModelIndex startIdx=index(idx.row(),0);
        QModelIndex endIdx=index(idx.row(),columnCount()-1);

        Q_EMIT dataChanged(startIdx,endIdx);

        bool checked=(value.toInt() == Qt::Checked)?true:false;
        Q_EMIT checkStateChanged(idx.row(),checked);
        return true;
    }
    return false;
}

QVariant LogLoadSuiteModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
    {
        switch ( section )
        {
        case 0: return tr("Suite");
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    else if(role== Qt::ToolTipRole)
    {
        switch ( section )
        {
        case 0: return tr("Suite");
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    return QVariant();
}

QModelIndex LogLoadSuiteModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex LogLoadSuiteModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

QString LogLoadSuiteModel::formatPrecentage(float perc) const
{
    if(perc < 0.5)
        return "<0.5";

    return QString::number(perc,'f',1);
}
#endif
