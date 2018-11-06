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
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QPainter>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

#include "SessionHandler.hpp"
#include "TextFormat.hpp"
#include "TimelineData.hpp"
#include "TimelineInfoDelegate.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "VNState.hpp"
#include "WidgetNameProvider.hpp"

#include "ui_TimelineInfoWidget.h"

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

void TimelineInfoModel::setData(TimelineItem *data,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
                                unsigned int endDateSec)
{
    beginResetModel();
    viewStartDateSec_=viewStartDateSec;
    viewEndDateSec_=viewEndDateSec;
    endDateSec_=endDateSec;
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
            return TimelineItem::toQDateTime(data_->start_[row]).toString("hh:mm:ss dd-MMM-yyyy");
        }
        else if(index.column() == 1)
        {            
            if(VNState* vn=VNState::find(data_->status_[row]))
            {
                return vn->name();
            }
        }
        else if(index.column() == 2)
        {
            qint64 dSec=-1;
            if(row != static_cast<int>(data_->size())-1)
            {
                dSec=data_->start_[row+1]-data_->start_[row];
            }
            else
            {
                dSec=endDateSec_-data_->start_[row];
            }

            if(dSec >=0)
            {
                if(dSec < 60)
                    return QString::number(dSec)  + "s";
                else if (dSec < 3600)
                    return QString::number(dSec / 60)  + "m " + QString::number(dSec % 60)  + "s";
                else
                    return "> 1h";
            }
        }
    }

    if(role == Qt::BackgroundRole)
    {
       if(index.column() == 1)
       {          
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
            return "Date";
        case 1:
            return "State";
        case 2:
            return "Duration in state";
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

bool TimelineInfoWidget::columnsAdjusted_=false;

TimelineInfoWidget::TimelineInfoWidget(QWidget *parent) :
    ui_(new Ui::TimelineInfoWidget),
    numOfRows_(0)
{
    ui_->setupUi(this);

    //title
    ui_->titleLabel->setProperty("fileInfo","1");
    ui_->titleLabel->setWordWrap(true);
    ui_->titleLabel->setMargin(2);
    ui_->titleLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    ui_->titleLabel->setAutoFillBackground(true);
    ui_->titleLabel->setFrameShape(QFrame::StyledPanel);
    ui_->titleLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    ui_->summaryLabel->hide();

    TimelineInfoDelegate *delegate=new TimelineInfoDelegate(this);
    ui_->timeTree->setItemDelegate(delegate);

    model_=new TimelineInfoModel(this);
    ui_->timeTree->setModel(model_);
}

void TimelineInfoWidget::load(QString host, QString port,TimelineData *tlData, int itemIndex,QDateTime viewStartDate,
                              QDateTime viewEndDate)
{
    Q_ASSERT(tlData);
    host_=host;
    port_=port;
    data_=tlData->items()[itemIndex];

    QColor col(39,49,101);
    QColor colText(30,30,30);
    QString title=Viewer::formatBoldText("Node:",col) + QString::fromStdString(data_.path());

    ui_->titleLabel->setText(title);

    model_->setData(&data_,viewStartDate.toMSecsSinceEpoch()/1000,
                    viewEndDate.toMSecsSinceEpoch()/1000,tlData->endTime());

    if(!columnsAdjusted_)
    {
        ui_->timeTree->resizeColumnToContents(0);
        ui_->timeTree->resizeColumnToContents(1);
        columnsAdjusted_=true;
    }
}

void TimelineInfoWidget::readSettings(QSettings& settings)
{
    if(settings.contains("timeTreeColumnWidth"))
    {
        QStringList lst=settings.value("timeTreeColumnWidth").toStringList();
        for(int i=0; i < lst.count(); i++)
            ui_->timeTree->setColumnWidth(i,lst[i].toInt());

        if(lst.count() >= 2)
            columnsAdjusted_=true;
    }
}

void TimelineInfoWidget::writeSettings(QSettings& settings)
{
    QStringList colW;
    for(int i=0; i < model_->columnCount()-1; i++)
        colW << QString::number(ui_->timeTree->columnWidth(i));

    settings.setValue("timeTreeColumnWidth",colW);
}

//=======================================================
//
// TimelineInfoDialog
//
//=======================================================

TimelineInfoDialog::TimelineInfoDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("ecFlowUI - Timeline details"));

    QVBoxLayout *vb=new QVBoxLayout(this);
    vb->setContentsMargins(4,4,4,4);

    infoW_=new TimelineInfoWidget(this);
    vb->addWidget(infoW_);

    QDialogButtonBox *buttonBox=new QDialogButtonBox(this);
    vb->addWidget(buttonBox);

    buttonBox->addButton(QDialogButtonBox::Close);

    connect(buttonBox,SIGNAL(rejected()),
            this,SLOT(reject()));

    readSettings();

    WidgetNameProvider::nameChildren(this);
}

TimelineInfoDialog::~TimelineInfoDialog()
{
    writeSettings();
}

void TimelineInfoDialog::closeEvent(QCloseEvent * event)
{
    event->accept();
    writeSettings();
}

void TimelineInfoDialog::writeSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("TimelineInfoDialog")),
                       QSettings::NativeFormat);

    //We have to clear it so that should not remember all the previous values
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size",size());

    infoW_->writeSettings(settings);

    settings.endGroup();
}

void TimelineInfoDialog::readSettings()
{
    SessionItem* cs=SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("TimelineInfoDialog")),
                       QSettings::NativeFormat);

    settings.beginGroup("main");
    if(settings.contains("size"))
    {
        resize(settings.value("size").toSize());
    }
    else
    {
        resize(QSize(480,420));
    }

    infoW_->readSettings(settings);

    settings.endGroup();
}
