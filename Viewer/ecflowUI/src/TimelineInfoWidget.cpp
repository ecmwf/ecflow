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
#include <QBuffer>
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
#include "TimelineHeaderView.hpp"
#include "TimelineInfoDelegate.hpp"
#include "TimelineModel.hpp"
#include "TimelineView.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
#include "VNState.hpp"
#include "WidgetNameProvider.hpp"

#include "ui_TimelineInfoWidget.h"

//=======================================================
//
// TimelineInfoModel
//
//=======================================================

TimelineInfoModel::TimelineInfoModel(QObject *parent) :
          QAbstractItemModel(parent),
          data_(0), firstRowInPeriod_(-1), lastRowInPeriod_(-1)
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
    determineRowsInPeriod();
    endResetModel();
}


void TimelineInfoModel::determineRowsInPeriod()
{
    firstRowInPeriod_=-1;
    lastRowInPeriod_=-1;

    if(!data_)
        return;

    for(unsigned int i=0; i < data_->size(); i++)
    {
        if(viewStartDateSec_ <= data_->start_[i])
        {
            firstRowInPeriod_=i;
            break;
        }
        if(i <  data_->size()-1 && viewStartDateSec_ < data_->start_[i+1])
        {
            firstRowInPeriod_=i+1;
            break;
        }
    }

    if(firstRowInPeriod_ != -11)
    {
        for(unsigned int i=static_cast<unsigned int>(firstRowInPeriod_); i < data_->size(); i++)
        {
            if(viewEndDateSec_ <= data_->start_[i])
            {
                lastRowInPeriod_=i-1;
                break;
            }
        }
    }
}

void TimelineInfoModel::clearData()
{
    beginResetModel();
    data_=0;
    firstRowInPeriod_=-1;
    lastRowInPeriod_=-1;
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
                return ViewerUtil::formatDuration(dSec);
            }
        }
    }

    else if(role == Qt::BackgroundRole)
    {
       if(index.column() == 1)
       {          
            if(VNState* vn=VNState::find(data_->status_[row]))
            {
                return vn->colour();
            }
       }
    }

    else if(role == Qt::UserRole)
    {
        if(firstRowInPeriod_!= -1)
        {
            if(row > firstRowInPeriod_ && (row < lastRowInPeriod_ || lastRowInPeriod_ == -1))
            {
                return 0;
            }
            else if(row == firstRowInPeriod_ )
            {
                return (row==0)?0:1;

            }
            else if(row == lastRowInPeriod_ )
            {
                return (row==static_cast<int>(data_->size()-1))?0:2;

            }
            else
            {
                return 3;
            }
        }

        return 3;
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
// TimelineInfoDailyModel
//
//=======================================================

TimelineInfoDailyModel::TimelineInfoDailyModel(QObject *parent) :
          QAbstractItemModel(parent), data_(0)
{
}

TimelineInfoDailyModel::~TimelineInfoDailyModel()
{
}

void TimelineInfoDailyModel::setData(TimelineItem *data,unsigned int viewStartDateSec,unsigned int viewEndDateSec,
                                unsigned int endDateSec)
{
    beginResetModel();
    viewStartDateSec_=viewStartDateSec;
    viewEndDateSec_=viewEndDateSec;
    endDateSec_=endDateSec;
    data_=data;
    days_.clear();
    if(data_)
    {
        data_->days(days_);
    }
    endResetModel();
}

void TimelineInfoDailyModel::clearData()
{
    beginResetModel();
    data_=0;
    days_.clear();
    endResetModel();
}

bool TimelineInfoDailyModel::hasData() const
{
    return (data_ != NULL);
}

int TimelineInfoDailyModel::columnCount( const QModelIndex& /*parent */) const
{
     return 2;
}

int TimelineInfoDailyModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return static_cast<int>(days_.size());
    }

    return 0;
}

Qt::ItemFlags TimelineInfoDailyModel::flags ( const QModelIndex & index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TimelineInfoDailyModel::data(const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }

    int row=index.row();
    if(row < 0 || row >= static_cast<int>(days_.size()))
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        if(index.column() == 0)
        {
            return TimelineItem::toQDateTime(days_[row]).toString("dd-MMM-yyyy");
        }
        else if(index.column() == 1)
        {
            return days_[row];

        }
    }

    return QVariant();
}

QVariant TimelineInfoDailyModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
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
            return "Daily cycle";
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QModelIndex TimelineInfoDailyModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex TimelineInfoDailyModel::parent(const QModelIndex &child) const
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
    numOfRows_(0),
    tlEndTime_(0)
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

    //Time tree
    TimelineInfoDelegate *delegate=new TimelineInfoDelegate(this);
    ui_->timeTree->setItemDelegate(delegate);

    model_=new TimelineInfoModel(this);
    ui_->timeTree->setModel(model_);

    //Daily tree
    ui_->dailyToolbar->hide(); //we will add zoom buttons to it

    ui_->dailyTree->setRootIsDecorated(false);
    ui_->dailyTree->setSortingEnabled(false);
    ui_->dailyTree->setAutoScroll(true);
    ui_->dailyTree->setAllColumnsShowFocus(true);
    ui_->dailyTree->setUniformRowHeights(true);
    ui_->dailyTree->setAlternatingRowColors(false);
    ui_->dailyTree->setMouseTracking(true);
    ui_->dailyTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    dailyModel_=new TimelineInfoDailyModel(this);
    ui_->dailyTree->setModel(dailyModel_);

    dailyHeader_=new NodeTimelineHeader(ui_->dailyTree);
    ui_->dailyTree->setHeader(dailyHeader_);

    TimelineInfoDailyDelegate *dailyDelegate=new TimelineInfoDailyDelegate(dailyModel_,this);
    ui_->dailyTree->setItemDelegate(dailyDelegate);
}

void TimelineInfoWidget::load(QString host, QString port,TimelineData *tlData, int itemIndex,QDateTime viewStartDate,
                              QDateTime viewEndDate)
{
    Q_ASSERT(tlData);
    host_=host;
    port_=port;
    data_=tlData->items()[itemIndex];
    tlEndTime_=tlData->endTime();

    QColor col(39,49,101);
    QColor colText(30,30,30);
    QString title=Viewer::formatBoldText("Node: ",col) + QString::fromStdString(data_.path());

    ui_->titleLabel->setText(title);

    model_->setData(&data_,viewStartDate.toMSecsSinceEpoch()/1000,
                    viewEndDate.toMSecsSinceEpoch()/1000,tlData->endTime());

    dailyModel_->setData(&data_,viewStartDate.toMSecsSinceEpoch()/1000,
                    viewEndDate.toMSecsSinceEpoch()/1000,tlData->endTime());

    if(!columnsAdjusted_)
    {
        ui_->timeTree->resizeColumnToContents(0);
        ui_->timeTree->resizeColumnToContents(1);
        columnsAdjusted_=true;
    }

    int first=data_.firstInPeriod(viewStartDate,viewEndDate);
    if(first != -1)
        ui_->timeTree->setCurrentIndex(model_->index(first-1,0));

    //Set css for the text formatting
    QString cssDoc="td {padding-left: 3px; paddig-top: 1px; padding-bottom: 1px; background-color: #F3F3F3;color: #323232;} \
                    td.title {padding-left: 2px; padding-top: 1px; padding-bottom: 1px;background-color: #e3e6f3; color: #323232;}";

    ui_->summaryTe->setReadOnly(false);
    ui_->summaryTe->document()->setDefaultStyleSheet(cssDoc);

    //TextBrowser
    createSummary();
}

void TimelineInfoWidget::createSummary()
{
    QString s;

    s+="<table width=\'100%\'>";

    s+="<tr><td class=\'title\' align=\'center\' colspan=\'2\' bgcolor=\'"  + QColor(238,238,238).name() +
            "\'><b>Statistics for the whole log period</b></td></tr>";

    createSummary(s,VNState::find("active"));
    createSummary(s,VNState::find("submitted"));
    createSummary(s,VNState::find("complete"));
    createSummary(s,VNState::find("aborted"));
    createSummary(s,VNState::find("queued"));

    s+="</table>";

    ui_->summaryTe->setHtml(s);
}

void TimelineInfoWidget::createSummary(QString &txt,VNState* state)
{
    if(!state)
        return;

    int num=0;
    float mean=0.;
    TimelineItemStats stats;
    unsigned char statusId=state->ucId();
    data_.durationStats(statusId,num,mean,stats,tlEndTime_);

    if(num <=0)
        return;

    txt+="<tr><td class=\'title\' bgcolor=\'" + state->colour().name() + "\' align=\'center\' colspan=\'2\' >" +
            Viewer::formatText(state->label(),state->fontColour()) + "</td></tr>";

    txt+=Viewer::formatTableRow("Number",QString::number(num),true);
    txt+=Viewer::formatTableRow("Total duration",ViewerUtil::formatDuration(stats.total),true);
    txt+=Viewer::formatTableRow("Minimum",ViewerUtil::formatDuration(stats.min),true);
    txt+=Viewer::formatTableRow("Maximum",ViewerUtil::formatDuration(stats.max),true);
    txt+=Viewer::formatTableRow("Mean",ViewerUtil::formatDuration(mean),true);

    if(num >=4 && stats.max-stats.min >= 3)
    {
        QPixmap pix=makeBoxPlot(state,num,mean,stats);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        pix.save(&buffer, "PNG");

        txt+="<tr><td align=\'center\' colspan=\'2\'><img src=\"data:image/png;base64," +
            byteArray.toBase64() + "\"/></td></tr>";
    }
}

QPixmap TimelineInfoWidget::makeBoxPlot(VNState* state, int num,int mean,TimelineItemStats stats)
{
    int w=200;
    int h=36;
    int xPadding=10;
    int extrH=12;
    int extrTop=(h-extrH)/2;
    int extrBottom=h-extrTop;
    int boxH=20;
    int boxTop=(h-boxH)/2;
    int boxBottom=h-boxTop;
    QColor col=state->colour();

    QPixmap pix(w,h);
    pix.fill(QColor(243,243,243));
    QPainter painter(&pix);
    painter.setPen(QPen(col.darker(140),2));
    painter.setBrush(col);

    float xRate=static_cast<float>(w-2*xPadding)/static_cast<float>(stats.max-stats.min);
    int x25=xPadding+static_cast<float>(stats.perc25-stats.min)*xRate;
    int x50=xPadding+static_cast<float>(stats.median-stats.min)*xRate;
    int x75=xPadding+static_cast<float>(stats.perc75-stats.min)*xRate;

    painter.drawLine(QPoint(xPadding,extrTop),QPoint(xPadding,extrBottom));
    painter.drawLine(QPoint(w-xPadding,extrTop),QPoint(w-xPadding,extrBottom));
    painter.drawLine(QPoint(xPadding,h/2),QPoint(w-xPadding,h/2));
    painter.fillRect(QRect(QPoint(x25,boxTop),QPoint(x75,boxBottom)),col.darker(110));
    painter.fillRect(QRect(QPoint(x50-1,boxTop),QPoint(x50+1,boxBottom)),col.darker(120));
    return pix;
}

void TimelineInfoWidget::readSettings(QSettings& settings)
{
    if(settings.contains("timeTreeColumnWidth"))
    {
        if(ViewerUtil::initTreeColumnWidth(settings,"timeTreeColumnWidth",ui_->timeTree))
        {
            columnsAdjusted_=true;
        }
    }

    int idx=settings.value("tab",0).toInt();
    if(idx >=0 && idx < ui_->tabWidget->count())
        ui_->tabWidget->setCurrentIndex(idx);
}

void TimelineInfoWidget::writeSettings(QSettings& settings)
{
    ViewerUtil::saveTreeColumnWidth(settings,"timeTreeColumnWidth",ui_->timeTree);
    settings.setValue("tab",ui_->tabWidget->currentIndex());
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
