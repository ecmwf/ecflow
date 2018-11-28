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
        if(viewStartDateSec_ > data_->start_[row] ||
           viewEndDateSec_ < data_->start_[row])
        {
            return QColor(240,240,240,116);
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
    QString title=Viewer::formatBoldText("Node: ",col) + QString::fromStdString(data_.path());

    ui_->titleLabel->setText(title);

    model_->setData(&data_,viewStartDate.toMSecsSinceEpoch()/1000,
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
    QString cssDoc="td {padding-left: 3px; paddig-top: 1px; padding-bottom: 1px; background-color: #F3F3F3;color: #000000;} \
                    td.title {padding-left: 2px; padding-top: 1px; padding-bottom: 1px;background-color: #e3e6f3; color: #000000;}";


    ui_->summaryTe->document()->setDefaultStyleSheet(cssDoc);

    //TextBrowser
    createSummary();
}

void TimelineInfoWidget::createSummary()
{
    QString s;

    s+="<table width=\'100%\'>";

    createSummary(s,VNState::find("active"));
    createSummary(s,VNState::find("submitted"));
    createSummary(s,VNState::find("complete"));

    s+="</table>";


#if 0
    s=Viewer::formatBoldText("Active",Qt::black);

    int num=0;
    float mean=0.;
    TimelineItemStats stats;
    VNState *state=VNState::find("active");
    unsigned char statusId=state->ucId();

    data_.durationStats(statusId,num,mean,stats);

    QColor bg(233,233,233);
    QColor fg(50,51,52);

    s+="<table width=\'100%\'>";
    s+=Viewer::formatTableRow("Number",QString::number(num),bg,fg,true);
    s+=Viewer::formatTableRow("Total duration",ViewerUtil::formatDuration(stats.total),bg,fg,true);
    s+=Viewer::formatTableRow("Minimum",ViewerUtil::formatDuration(stats.min),bg,fg,true);
    s+=Viewer::formatTableRow("Maximum",ViewerUtil::formatDuration(stats.max),bg,fg,true);
    s+=Viewer::formatTableRow("Mean",ViewerUtil::formatDuration(mean),bg,fg,true);
    s+=Viewer::formatTableRow("Median",ViewerUtil::formatDuration(stats.median),bg,fg,true);

    //s+="<tr><td>Num</td><td>" + QString::number(num) + "</td></tr>";
    //s+="</table>";

    QPixmap pix=makeBoxPlot(state,num,mean,stats);
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    pix.save(&buffer, "PNG");

    s +="<tr><td colspan=\'2\'><img src=\"data:image/png;base64," +
            byteArray.toBase64() + "\"/></td></tr>";

    s+="</table>";
#endif

    ui_->summaryTe->setHtml(s);
}

void TimelineInfoWidget::createSummary(QString &txt,VNState* state)
{
    if(!state)
        return;

    QColor bg(233,233,233);
    QColor fg(50,51,52);

    txt+="<tr><td colspan=\'2\'>" + Viewer::formatBoldText("Active",fg) + "</td></tr>";

    int num=0;
    float mean=0.;
    TimelineItemStats stats;
    unsigned char statusId=state->ucId();

    data_.durationStats(statusId,num,mean,stats);

    txt+=Viewer::formatTableRow("Number",QString::number(num),bg,fg,true);

    if(num <=0)
        return;

    txt+=Viewer::formatTableRow("Total duration",ViewerUtil::formatDuration(stats.total),bg,fg,true);
    txt+=Viewer::formatTableRow("Minimum",ViewerUtil::formatDuration(stats.min),bg,fg,true);
    txt+=Viewer::formatTableRow("Maximum",ViewerUtil::formatDuration(stats.max),bg,fg,true);
    txt+=Viewer::formatTableRow("Mean",ViewerUtil::formatDuration(mean),bg,fg,true);
    txt+=Viewer::formatTableRow("Median",ViewerUtil::formatDuration(stats.median),bg,fg,true);

    if(num >=3)
    {
        QPixmap pix=makeBoxPlot(state,num,mean,stats);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        pix.save(&buffer, "PNG");

        txt+="<tr><td colspan=\'2\'><img src=\"data:image/png;base64," +
            byteArray.toBase64() + "\"/></td></tr>";
    }
}

QPixmap TimelineInfoWidget::makeBoxPlot(VNState* state, int num,int mean,TimelineItemStats stats)
{
    int w=200;
    int h=50;
    int xPadding=10;
    int yPadding=15;
    int boxH=25;
    int boxTop=(h-boxH)/2;
    int boxBottom=h-boxTop;
    QColor col=state->colour();

    QPixmap pix(w,h);
    pix.fill(QColor(239,239,239));
    QPainter painter(&pix);
    painter.setPen(QPen(col.darker(140),2));
    painter.setBrush(col);

    float xRate=static_cast<float>(w-2*xPadding)/static_cast<float>(stats.max-stats.min);
    int x25=xPadding+static_cast<float>(stats.perc25-stats.min)*xRate;
    int x50=xPadding+static_cast<float>(stats.median-stats.min)*xRate;
    int x75=xPadding+static_cast<float>(stats.perc75-stats.min)*xRate;

    painter.drawLine(QPoint(xPadding,yPadding),QPoint(xPadding,h-yPadding));
    painter.drawLine(QPoint(w-xPadding,yPadding),QPoint(w-xPadding,h-yPadding));
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
}

void TimelineInfoWidget::writeSettings(QSettings& settings)
{
    ViewerUtil::saveTreeColumnWidth(settings,"timeTreeColumnWidth",ui_->timeTree);
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
