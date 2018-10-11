//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LogLoadWidget.hpp"

#include "File_r.hpp"
#include "File.hpp"
#include "LogModel.hpp"
#include "NodePath.hpp"
#include "Str.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBox>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

#include "ui_LogLoadWidget.h"

//=======================================================
//
// LogLoadWidget
//
//=======================================================

LogLoadWidget::LogLoadWidget(QWidget *parent) :
    ui_(new Ui::LogLoadWidget),
    numOfRows_(0)
{
    ui_->setupUi(this);

    //message label
    ui_->messageLabel->hide();

    //Chart views
    viewHandler_=new LogRequestViewHandler(this);
    for(int i=0; i < viewHandler_->tabItems().count(); i++)
    {
        ui_->viewTab->addTab(viewHandler_->tabItems()[i]," h");
    }

    ui_->viewTab->setTabText(0,tr("Total charts"));
    ui_->viewTab->setTabText(1,tr("Other charts"));
    ui_->viewTab->setTabText(2,tr("Tables"));

    ui_->viewTab->setCurrentIndex(0);

    //Cornerbutton for tab
    //QWidget *cornerW=new QWidget(this);
    //QHBoxLayout *cornerHb=new QHBoxLayout(cornerW);
    //cornerHb->setContentsMargins(0,0,0,0);

    //QToolButton* showFullTb=new QToolButton(this);
    //showFullTb->setText(tr("Full period"));
    //cornerHb->addWidget(showFullTb);

    connect(ui_->showFullTb,SIGNAL(clicked()),
            viewHandler_,SLOT(showFullRange()));

    //Temporal resolution combo box
    //resCombo_=new QComboBox(this);
    ui_->resCombo->addItem("seconds",0);
    ui_->resCombo->addItem("minutes",1);
    ui_->resCombo->addItem("hours",2);

    connect(ui_->resCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(resolutionChanged(int)));

    //cornerHb->addWidget(new QLabel("Resolution:",this));
    //cornerHb->addWidget(ui_->resCombo_);

    //ui_->viewTab->setCornerWidget(cornerW);


    //Log contents
    logModel_=new LogModel(this);

    ui_->logView->setProperty("log","1");
    ui_->logView->setProperty("log","1");
    ui_->logView->setRootIsDecorated(false);
    ui_->logView->setLogModel(logModel_);
    ui_->logView->setUniformRowHeights(true);
    ui_->logView->setAlternatingRowColors(false);
    ui_->logView->setItemDelegate(new LogDelegate(this));
    ui_->logView->setContextMenuPolicy(Qt::ActionsContextMenu);

    //make the horizontal scrollbar work
    ui_->logView->header()->setStretchLastSection(false);
    ui_->logView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    //Define context menu
    //ui_->logView->addAction(actionCopyEntry_);
    //ui_->logView->addAction(actionCopyRow_);

    //Connect the views to the log model
    connect(viewHandler_,SIGNAL(timeRangeChanged(qint64,qint64)),
            logModel_,SLOT(setPeriod(qint64,qint64)));

    connect(viewHandler_,SIGNAL(timeRangeChanged(qint64,qint64)),
            this,SLOT(periodChanged(qint64,qint64)));

    connect(viewHandler_, SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)),
            logModel_,SLOT(setHighlightPeriod(qint64,qint64,qint64)));

    connect(viewHandler_,SIGNAL(timeRangeReset()),
            logModel_,SLOT(resetPeriod()));

    connect(viewHandler_,SIGNAL(timeRangeReset()),
            this,SLOT(periodWasReset()));

    //logInfo label
    ui_->logInfoLabel->setProperty("fileInfo","1");
    ui_->logInfoLabel->setWordWrap(true);
    ui_->logInfoLabel->setMargin(2);
    ui_->logInfoLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    ui_->logInfoLabel->setAutoFillBackground(true);
    ui_->logInfoLabel->setFrameShape(QFrame::StyledPanel);
    ui_->logInfoLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);


    connect(ui_->reloadTb,SIGNAL(clicked()),
            this,SLOT(slotReload()));

    //ui_->timeWidget->setStyleSheet("#timeWidget{background-color: rgb(212,212,212);}");
}

LogLoadWidget::~LogLoadWidget()
{
}

void LogLoadWidget::clear()
{
    ui_->messageLabel->clear();
    ui_->messageLabel->hide();

    ui_->logInfoLabel->setText(QString());
    viewHandler_->clear();

    logModel_->clearData();
    logFile_.clear();
    serverName_.clear();
    host_.clear();
    port_.clear();
    ui_->startTe->clear();
    ui_->endTe->clear();

    setAllVisible(false);
}

void LogLoadWidget::updateInfoLabel()
{
    QColor col(39,49,101);
    QString txt=Viewer::formatBoldText("Log file: ",col) + logFile_;
    txt+=Viewer::formatBoldText(" Server: ",col) + serverName_ +
         Viewer::formatBoldText(" Host: ",col) + host_ +
         Viewer::formatBoldText(" Port: ",col) + port_;


    QDateTime startDt=viewHandler_->data()->startTime();
    QDateTime endDt=viewHandler_->data()->endTime();
    txt+=Viewer::formatBoldText(" Full period: ",col) +
            startDt.toString("yyyy-MM-dd hh:mm:ss") + Viewer::formatBoldText(" to ",col) +
            endDt.toString("yyyy-MM-dd hh:mm:ss");

    int maxNum=viewHandler_->data()->maxNumOfRows();
    int num=viewHandler_->data()->numOfRows();
    if(maxNum != 0 && num == abs(maxNum))
    {
        txt+=Viewer::formatBoldText(" Log entries: ",col) +
           "last " + QString::number(abs(maxNum)) + " rows read (maximum reached)";
    }

    ui_->logInfoLabel->setText(txt);

    //TODO: we need a better implementation
    ui_->timeWidget->hide();
}

void LogLoadWidget::setAllVisible(bool b)
{
    ui_->viewTab->setVisible(b);
    ui_->logView->setVisible(b);
    ui_->timeWidget->setVisible(b);
}

void LogLoadWidget::slotReload()
{
    if(!serverName_.isEmpty() && numOfRows_ != 0)
    {
        load(serverName_, host_, port_, logFile_,numOfRows_);
    }
}

void LogLoadWidget::load(QString logFile,int numOfRows)
{
    load("","","",logFile,numOfRows);
}

void LogLoadWidget::load(QString serverName, QString host, QString port, QString logFile,int numOfRows)
{
    clear();

    serverName_=serverName;
    host_=host;
    port_=port;
    logFile_=logFile;
    numOfRows_=numOfRows;

    updateInfoLabel();

    QFileInfo fInfo(logFile);
    if(!fInfo.exists())
    {
        ui_->messageLabel->showError("The specified log file does not exist!");
        return;
    }

    if(!fInfo.isReadable())
    {
        ui_->messageLabel->showError("The specified log file is not readable!");
        return;
    }

    if(!fInfo.isFile())
    {
        ui_->messageLabel->showError("The specified log file is not a file!");
        return;
    }

    setAllVisible(true);

    try
    {
        viewHandler_->load(logFile_.toStdString(),numOfRows);
    }
    catch(std::runtime_error e)
    {
        ui_->messageLabel->showError(e.what());
        setAllVisible(false);
    }

    logModel_->loadFromFile(logFile_.toStdString(),viewHandler_->data()->startPos());

    QDateTime startTime=viewHandler_->data()->startTime();
    QDateTime endTime=viewHandler_->data()->endTime();
    ui_->startTe->setMinimumDateTime(startTime);
    ui_->startTe->setMaximumDateTime(endTime);
    ui_->startTe->setDateTime(startTime);
    ui_->endTe->setMinimumDateTime(startTime);
    ui_->endTe->setMaximumDateTime(endTime);
    ui_->endTe->setDateTime(endTime);

    updateInfoLabel();
}

void LogLoadWidget::periodChanged(qint64 start,qint64 end)
{
    QDateTime startDt=QDateTime::fromMSecsSinceEpoch(start);
    QDateTime endDt=QDateTime::fromMSecsSinceEpoch(end);
    ui_->startTe->setDateTime(startDt);
    ui_->endTe->setDateTime(endDt);
}

void LogLoadWidget::periodWasReset()
{
    QDateTime startDt=viewHandler_->data()->startTime();
    QDateTime endDt=viewHandler_->data()->endTime();
    ui_->startTe->setDateTime(startDt);
    ui_->endTe->setDateTime(endDt);
}

void LogLoadWidget::resolutionChanged(int)
{
    int idx=ui_->resCombo->currentIndex();
    if(idx == 0)
        viewHandler_->setResolution(LogLoadData::SecondResolution);
    else if(idx == 1)
        viewHandler_->setResolution(LogLoadData::MinuteResolution);
    else if(idx == 2)
        viewHandler_->setResolution(LogLoadData::HourResolution);
}

LogLoadRequestSortModel::LogLoadRequestSortModel(QObject* parent) : QSortFilterProxyModel(parent)
{

}

bool LogLoadRequestSortModel::lessThan(const QModelIndex &left,
                                        const QModelIndex &right) const
{
    if(left.column() == 0)
    {
        QString leftData = sourceModel()->data(left).toString();
        QString rightData = sourceModel()->data(right).toString();

        return QString::localeAwareCompare(leftData, rightData) < 0;
    }
    else
    {
        qreal leftData = sourceModel()->data(left,Qt::UserRole).toFloat();
        qreal rightData = sourceModel()->data(right,Qt::UserRole).toFloat();

        if(leftData == rightData)
        {
            QModelIndex leftIdx=sourceModel()->index(left.row(),0);
            QModelIndex rightIdx=sourceModel()->index(right.row(),0);

            QString leftString = sourceModel()->data(leftIdx).toString();
            QString rightString = sourceModel()->data(rightIdx).toString();

            return QString::localeAwareCompare(leftString, rightString) < 0;
        }

        else
           return leftData < rightData;

    }

    return false;
}

//=====================================================
//
//  LogLoadRequestModel
//
//=====================================================

LogLoadRequestModel::LogLoadRequestModel(QString dataName,QObject *parent) :
          QAbstractItemModel(parent),
          dataName_(dataName),
          showColour_(true)
{
}

LogLoadRequestModel::~LogLoadRequestModel()
{
}



void LogLoadRequestModel::setData(const std::vector<LogRequestItem>& data,QList<bool> checkedLst)
{
    //Q_ASSERT(data);
    Q_ASSERT(data.size() == static_cast<size_t>(checkedLst.size()));

    beginResetModel();

    data_.clear();
    for(size_t i=0; i < data.size(); i++)
    {
        data_ << LogLoadRequestModelDataItem(QString::fromStdString(data[i].name_),
                                           data[i].periodStat().percentage_,checkedLst[i],
                                           data[i].periodStat().rank_);
    }
    endResetModel();
}

void LogLoadRequestModel::setData(const std::vector<LogLoadDataItem>& data,QList<bool> checkedLst)
{
    //Q_ASSERT(data);
    Q_ASSERT(data.size() == static_cast<size_t>(checkedLst.size()));

    beginResetModel();

    data_.clear();
    for(size_t i=0; i < data.size(); i++)
    {
        data_ << LogLoadRequestModelDataItem(QString::fromStdString(data[i].name()),
                                           data[i].periodStat().percentage(),checkedLst[i],
                                           data[i].periodStat().rank());
    }
    endResetModel();
}

void LogLoadRequestModel::adjustStats(const std::vector<LogRequestItem>& data)
{
    //Q_ASSERT(data);
    Q_ASSERT(data_.size() == static_cast<int>(data.size()));
    for(int i=0; i < data_.size(); i++)
    {
        data_[i].percentage_=data[i].periodStat().percentage();
        data_[i].rank_=data[i].periodStat().rank();
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::adjustStats(const std::vector<LogLoadDataItem>& data)
{
    //Q_ASSERT(data);
    Q_ASSERT(data_.size() == static_cast<int>(data.size()));
    for(int i=0; i < data_.size(); i++)
    {
        data_[i].percentage_=data[i].periodStat().percentage();
        data_[i].rank_=data[i].periodStat().rank();
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::clearData()
{
    beginResetModel();
    data_.clear();
    endResetModel();
}

bool LogLoadRequestModel::hasData() const
{
    return !data_.isEmpty();
}

int LogLoadRequestModel::columnCount( const QModelIndex& /*parent */ ) const
{
     return 2;
}

int LogLoadRequestModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return data_.count();
    }

    return 0;
}

Qt::ItemFlags LogLoadRequestModel::flags ( const QModelIndex & index) const
{
    Qt::ItemFlags defaultFlags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if(index.column() == 0)
    {
        defaultFlags=defaultFlags | Qt::ItemIsUserCheckable;
    }
    return defaultFlags;
}

QVariant LogLoadRequestModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }
    int row=index.row();
    if(row < 0 || row >= data_.count())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0:
            return data_[row].name_;
            break;
        case 1:
            return formatPrecentage(data_[row].percentage_);
            break;
        default:
            break;
        }
    }
    else if (role == Qt::CheckStateRole)
    {
        if(index.column() == 0)
            return (data_[row].checked_)?QVariant(Qt::Checked):QVariant(Qt::Unchecked);

        return QVariant();
    }
    else if(role == Qt::UserRole)
    {
        switch(index.column())
        {
        case 0:
            return data_[row].name_;
            break;
        case 1:
            return data_[row].percentage_;
            break;
        default:
            break;
        }
    }
    else if(role == Qt::BackgroundRole && showColour_)
    {
        if(data_[row].checked_ && data_[row].col_ != QColor() )
            return data_[row].col_;

        return QVariant();
    }

    return QVariant();
}

bool LogLoadRequestModel::setData(const QModelIndex& idx, const QVariant & value, int role )
{
    if(idx.column() == 0 && role == Qt::CheckStateRole)
    {
        QModelIndex startIdx=index(idx.row(),0);
        QModelIndex endIdx=index(idx.row(),columnCount()-1);
        Q_EMIT dataChanged(startIdx,endIdx);

        bool checked=(value.toInt() == Qt::Checked)?true:false;
        data_[idx.row()].checked_=checked;
        Q_EMIT checkStateChanged(idx.row(),checked);

        return true;
    }
    return false;
}

QVariant LogLoadRequestModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if ( orient != Qt::Horizontal || (role != Qt::DisplayRole &&  role != Qt::ToolTipRole))
              return QAbstractItemModel::headerData( section, orient, role );

    if(role == Qt::DisplayRole)
    {
        switch ( section )
        {
        case 0: return dataName_;
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    else if(role== Qt::ToolTipRole)
    {
        switch ( section )
        {
        case 0: return dataName_;
        case 1: return tr("Request (%)");
        default: return QVariant();
        }
    }
    return QVariant();
}

QModelIndex LogLoadRequestModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex LogLoadRequestModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

QString LogLoadRequestModel::formatPrecentage(float perc) const
{
    if(perc < 0.000000001)
        return "0";
    else if(perc < 0.5)
        return "<0.5";

    return QString::number(perc,'f',1);
}

void LogLoadRequestModel::updateItem(int idx,bool st,QColor col)
{
    if(idx>=0 && idx < data_.size())
    {
        data_[idx].col_=col.lighter(150);
        QModelIndex startIdx=index(idx,0);
        QModelIndex endIdx=index(idx,columnCount()-1);
        Q_EMIT dataChanged(startIdx,endIdx);
    }
}

void LogLoadRequestModel::selectAll()
{
    for(int i=0; i < data_.size(); i++)
    {
        if(!data_[i].checked_)
        {
            data_[i].checked_=true;
            Q_EMIT checkStateChanged(i,true);
        }
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::unselectAll()
{
    for(int i=0; i < data_.size(); i++)
    {
        if(data_[i].checked_)
        {
            data_[i].checked_=false;
            Q_EMIT checkStateChanged(i,false);
        }
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::selectFirstFourItems()
{
    unselectAll();
    for(int i=0; i < data_.size(); i++)
    {
        if(data_[i].rank_ < 4 && data_[i].rank_ >=0)
        {
            data_[i].checked_=true;
            Q_EMIT checkStateChanged(i,true);
        }
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::selectFirstItem()
{
    unselectAll();
    for(int i=0; i < data_.size(); i++)
    {
        if(data_[i].rank_ ==0)
        {
            data_[i].checked_=true;
            Q_EMIT checkStateChanged(i,true);
        }
    }

    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

void LogLoadRequestModel::setShowColour(bool b)
{
    showColour_=b;
    QModelIndex startIdx=index(0,0);
    QModelIndex endIdx=index(rowCount(),columnCount()-1);
    Q_EMIT dataChanged(startIdx,endIdx);
}

//=============================================
//
// ChartCallout
//
//=============================================

ChartCallout::ChartCallout(QChart *chart):
    QGraphicsItem(chart),
    chart_(chart)
{
    font_.setPointSize(font_.pointSize()-1);
}

QRectF ChartCallout::boundingRect() const
{
    QPointF anchor = mapFromParent(chart_->mapToPosition(anchor_));
    QPointF bottom = mapFromParent(chart_->mapToPosition(bottomPos_));
    QRectF rect;
    rect.setLeft(qMin(rect_.left(), anchor.x()));
    rect.setRight(qMax(rect_.right(), anchor.x()));
    rect.setTop(qMin(rect_.top(), anchor.y()));
    rect.setBottom(bottom.y());
    return rect;
}

void ChartCallout::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    QPainterPath path;
    path.addRoundedRect(rect_, 5, 5);

    //The callout shape
    QPointF anchor = mapFromParent(chart_->mapToPosition(anchor_));
    if(!rect_.contains(anchor))
    {
        QPointF point1, point2;

        bool above = anchor.y() <= rect_.top();
        bool aboveCenter = anchor.y() > rect_.top() && anchor.y() <= rect_.center().y();
        bool belowCenter = anchor.y() > rect_.center().y() && anchor.y() <= rect_.bottom();
        bool below = anchor.y() > rect_.bottom();

        bool onLeft = anchor.x() <= rect_.left();
        bool leftOfCenter = anchor.x() > rect_.left() && anchor.x() <= rect_.center().x();
        bool rightOfCenter = anchor.x() > rect_.center().x() && anchor.x() <= rect_.right();
        bool onRight = anchor.x() > rect_.right();

        // get the nearest rect corner.
        qreal x = (onRight + rightOfCenter) * rect_.width();
        qreal y = (below + belowCenter) * rect_.height();
        bool cornerCase = (above && onLeft) || (above && onRight) || (below && onLeft) || (below && onRight);
        bool vertical = qAbs(anchor.x() - x) > qAbs(anchor.y() - y);

        qreal x1 = x + leftOfCenter * 10 - rightOfCenter * 20 + cornerCase * !vertical * (onLeft * 10 - onRight * 20);
        qreal y1 = y + aboveCenter * 5 - belowCenter * 10 + cornerCase * vertical * (above * 5 - below * 10);
        point1.setX(x1);
        point1.setY(y1);

        qreal x2 = x + leftOfCenter * 20 - rightOfCenter * 10 + cornerCase * !vertical * (onLeft * 20 - onRight * 10);
        qreal y2 = y + aboveCenter * 10 - belowCenter * 5 + cornerCase * vertical * (above * 10 - below * 5);
        point2.setX(x2);
        point2.setY(y2);

        path.moveTo(point1);
        path.lineTo(anchor);
        path.lineTo(point2);
        path = path.simplified();
    }

    painter->setBrush(QColor(198,223,188));
    //painter->setBrush(QColor(255,245,204));
    painter->drawPath(path);

    painter->setFont(font_);
    //painter->setPen(QColor(255, 255, 255));
    painter->drawText(textRect_, text_);

    //Vertical line down from the anchor pos
    painter->setPen(QPen(QColor(80,80,80),1,Qt::DotLine));
    painter->drawLine(anchor,mapFromParent(chart_->mapToPosition(bottomPos_)));
}

void ChartCallout::setText(const QString &text)
{
    text_ = text;
    QFontMetrics metrics(font_);
    textRect_ = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, text_);
    textRect_.translate(5, 5);
    prepareGeometryChange();
    rect_ = textRect_.adjusted(-5, -5, 5, 5);
}

void ChartCallout::setAnchor(QPointF point)
{
    anchor_ = point; //in value coords
    bottomPos_ = QPointF(point.x(),0); //in value coords
    updateGeometry();
}

void ChartCallout::updateGeometry()
{
    prepareGeometryChange();
    setPos(chart_->mapToPosition(anchor_) + QPoint(10, -30));
}

//=============================================
//
// ChartView
//
//=============================================

ChartView::ChartView(QChart *chart, QWidget *parent) :
    QChartView(chart, parent), callout_(0)
{
    setRubberBand(QChartView::HorizontalRubberBand);
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
    QChartView::mousePressEvent(event);
    if(event->button() == Qt::MidButton)
    {
       if(event->pos().x() <= chart()->plotArea().right() &&
          event->pos().x() >= chart()->plotArea().left())
       {
            qreal t=chart()->mapToValue(event->pos()).x();
            Q_EMIT positionClicked(t);
       }
       else
       {
            Q_EMIT positionClicked(0);
       }
    }
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    QChartView::mouseMoveEvent(event);

    if(event->pos().x() <= chart()->plotArea().right() &&
       event->pos().x() >= chart()->plotArea().left())
    {
        qreal v=chart()->mapToValue(event->pos()).x();
        Q_EMIT positionChanged(v);
    }
    else
        Q_EMIT positionChanged(-1);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    QPointF oriLeft=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF oriRight=chart()->mapToValue(chart()->plotArea().topRight());

    //UiLog().dbg() << "  " << chart()->mapToValue(chart()->plotArea().bottomLeft());

    QChartView::mouseReleaseEvent(event);

    //UiLog().dbg() << "   " << chart()->plotArea();
    //UiLog().dbg() << "  " << chart()->mapToValue(chart()->plotArea().bottomLeft());

    QPointF newLeft=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF newRight=chart()->mapToValue(chart()->plotArea().topRight());

    if(newLeft != oriLeft || newRight != oriRight )
    {
        Q_EMIT chartZoomed(QRectF(newLeft,newRight));
    }

    qint64 period=newRight.x()-newLeft.x(); //in ms
    adjustTimeAxis(period);
}

void ChartView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

void ChartView::doZoom(QRectF valRect)
{
    QRectF r(chart()->mapToPosition(valRect.bottomLeft()),
             chart()->mapToPosition(valRect.topRight()));

    if(r.isValid())
    {
        chart()->zoomIn(r);
        qint64 period=valRect.width(); //in ms
        adjustTimeAxis(period);
    }
}

void ChartView::doZoom(qint64 start,qint64 end)
{
    QPointF left=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF right=chart()->mapToValue(chart()->plotArea().topRight());

    QRectF valRect(QPointF(start,left.y()),QPointF(end,right.y()));
    QRectF r(chart()->mapToPosition(valRect.bottomLeft()),
             chart()->mapToPosition(valRect.topRight()));

    //QRectF r(chart()->mapToPosition(QPointF(start,left.y())),
    //         chart()->mapToPosition(QPointF(end,left.y()+1)));

    UiLog().dbg() << "start " << QDateTime::fromMSecsSinceEpoch(start);
    UiLog().dbg() << "end " << QDateTime::fromMSecsSinceEpoch(end);

    if(r.isValid())
    {
        chart()->zoomIn(r);
        qint64 period=end-start; //in ms
        adjustTimeAxis(period);
    }
}

void ChartView::currentTimeRange(qint64& start,qint64& end)
{
    start=chart()->mapToValue(chart()->plotArea().bottomLeft()).x();
    end=chart()->mapToValue(chart()->plotArea().topRight()).x();
}

qint64 ChartView::widthToTimeRange(float wPix)
{
    if(wPix > 0)
    {
        float pw=chart()->plotArea().width();
        if(pw >0)
        {
            qint64 start=chart()->mapToValue(chart()->plotArea().bottomLeft()).x();
            qint64 end=chart()->mapToValue(chart()->plotArea().topRight()).x();
            qint64 tw=(end-start)*(wPix/chart()->plotArea().width());
            return tw;
        }
    }
    return -1;
}

void ChartView::adjustTimeAxis(qint64 periodInMs)
{
    qint64 period=periodInMs/1000; //in seconds
    QString format;

    if(period < 60*60)
    {
        format="hh:mm:ss";
    }   
    else if(period < 2*24*3600)
    {
        format="hh:mm";
    }   
    else
    {
        format="dd MMM";
    }

    if(QDateTimeAxis *ax=static_cast<QDateTimeAxis*>(chart()->axisX()))
    {
        ax->setFormat(format);
    }
}

void ChartView::setCallout(qreal val)
{
    if(!callout_)
    {
        callout_=new ChartCallout(chart());
        scene()->addItem(callout_);
    }

    if(QValueAxis *axisY=static_cast<QValueAxis*>(chart()->axisY()))
    {
        qreal m=axisY->max();
        callout_->setAnchor(QPointF(val,m));
        QString txt=QDateTime::fromMSecsSinceEpoch(val).toString("hh:mm:ss dd/MM/yyyy");
        callout_->setText(txt);
    }
}

void ChartView::adjustCallout()
{
    if(callout_)
    {
        QPointF anchor=callout_->anchor();
        qint64 start,end;
        currentTimeRange(start,end);
        if(anchor.x() >= start && anchor.x() <=end)
            callout_->setAnchor(callout_->anchor());
        else
        {
            scene()->removeItem(callout_);
            delete callout_;
            callout_=0;
        }
    }
}

void ChartView::removeCallout()
{
    if(callout_)
    {
        scene()->removeItem(callout_);
        delete callout_;
        callout_=0;
    }
}

//=============================================
//
// LogRequestViewHandler
//
//=============================================

LogRequestViewHandler::LogRequestViewHandler(QWidget* parent) :
    data_(0), lastScanIndex_(0)
{
    //The data object - to read and store processed log data
    data_=new LogLoadData();

    LogRequestView* view;

    //tab: totals
    view=new LogTotalRequestView(this,parent);
    views_ << view;
    tabItems_ << view;

    //tab
    buildOtherTab(parent);

    //tab: tables
    buildTableTab(parent);

    for(int i=0; i < views_.count(); i++)
    {      
        connect(views_[i],SIGNAL(zoomHappened(QRectF)),
                this,SLOT(slotZoomHappened(QRectF)));

        connect(views_[i],SIGNAL(timeRangeChanged(qint64,qint64)),
                this,SIGNAL(timeRangeChanged(qint64,qint64)));

        connect(views_[i],SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)),
                this,SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)));

        connect(views_[i],SIGNAL(timeRangeReset()),
                this,SIGNAL(timeRangeReset()));
    }
}

LogRequestViewHandler::~LogRequestViewHandler()
{
    delete data_;
}

void LogRequestViewHandler::buildOtherTab(QWidget *parent)
{
    LogRequestView* view=new LogCmdSuiteRequestView(this,parent);
    views_ << view;

    view=new LogSuiteCmdRequestView(this,parent);
    views_ << view;

    view=new LogUidCmdRequestView(this,parent);
    views_ << view;

    view=new LogCmdUidRequestView(this,parent);
    views_ << view;

    QWidget* w=new QWidget(parent);
    QVBoxLayout* vb=new QVBoxLayout(w);
    vb->setContentsMargins(0,3,0,0);
    vb->setSpacing(1);

    QHBoxLayout* hb=new QHBoxLayout();

    QLabel *label=new QLabel("Mode: ",w);
    hb->addWidget(label);

    QComboBox* cb=new QComboBox(w);
    cb->addItem("Command graph per suite",0);
    cb->addItem("Suite graph per command",1);
    cb->addItem("User graph per command",2);
    cb->addItem("Command graph per user",3);
    hb->addWidget(cb);
    hb->addStretch(1);

    vb->addLayout(hb);

    QStackedWidget* stacked=new QStackedWidget(w);
    int cnt=views_.count();
    for(int i=cnt-4; i < cnt ; i++)
        stacked->addWidget(views_[i]);

    vb->addWidget(stacked);

    connect(cb,SIGNAL(currentIndexChanged(int)),
            stacked,SLOT(setCurrentIndex(int)));

    cb->setCurrentIndex(0);
    stacked->setCurrentIndex(0);

    tabItems_ << w;
}

void LogRequestViewHandler::buildTableTab(QWidget *parent)
{
    LogRequestView* view=new LogStatCmdUidView(this,parent);
    views_ << view;

    view=new LogStatUidCmdView(this,parent);
    views_ << view;

    view=new LogStatCmdSuiteView(this,parent);
    views_ << view;

    view=new LogStatSuiteCmdView(this,parent);
    views_ << view;

    QWidget* w=new QWidget(parent);
    QVBoxLayout* vb=new QVBoxLayout(w);
    vb->setContentsMargins(0,3,0,0);
    vb->setSpacing(1);

    QHBoxLayout* hb=new QHBoxLayout();

    QLabel *label=new QLabel("Mode: ",w);
    hb->addWidget(label);

    QComboBox* cb=new QComboBox(w);
    cb->addItem("command vs user",0);
    cb->addItem("user vs command",1);
    cb->addItem("command vs suite",2);
    cb->addItem("suite vs command",3);
    hb->addWidget(cb);
    hb->addStretch(1);

    vb->addLayout(hb);

    QStackedWidget* stacked=new QStackedWidget(w);
    int cnt=views_.count();
    for(int i=cnt-4; i < cnt ; i++)
        stacked->addWidget(views_[i]);

    vb->addWidget(stacked);

    connect(cb,SIGNAL(currentIndexChanged(int)),
            stacked,SLOT(setCurrentIndex(int)));

    cb->setCurrentIndex(0);
    stacked->setCurrentIndex(0);

    tabItems_ << w;
}

void LogRequestViewHandler::clear()
{
    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->clear();
    }
}

void LogRequestViewHandler::load(const std::string& logFile,int numOfRows)
{
    data_->loadLogFile(logFile,numOfRows);

    suitePlotState_.clear();
    for(size_t i=0; i < data_->suites().size(); i++)
        suitePlotState_ << false;

    cmdPlotState_.clear();
    for(size_t i=0; i < data_->total().subReq().size(); i++)
        cmdPlotState_ << false;

    uidPlotState_.clear();
    for(size_t i=0; i < data_->uidData().size(); i++)
        uidPlotState_ << false;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->load();
    }
}

void LogRequestViewHandler::setResolution(LogLoadData::TimeRes res)
{
     data_->setTimeRes(res);
     for(int i=0; i < views_.count(); i++)
     {
         views_[i]->changeResolution();
     }
}

void LogRequestViewHandler::showFullRange()
{
    data_->computeStat();

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->showFullRange();
    }
}

void LogRequestViewHandler::addRemoveSuite(int idx, bool st)
{
    suitePlotState_[idx]=st;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->addRemoveSuite(idx,st);
    }
}

void LogRequestViewHandler::addRemoveCmd(int idx, bool st)
{
    cmdPlotState_[idx]=st;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->addRemoveCmd(idx,st);
    }
}

void LogRequestViewHandler::addRemoveUid(int idx, bool st)
{
    uidPlotState_[idx]=st;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->addRemoveUid(idx,st);
    }
}


void LogRequestViewHandler::slotZoomHappened(QRectF r)
{
    if(LogRequestView* senderView=static_cast<LogRequestView*>(sender()))
    {       
        Q_FOREACH(LogRequestView* v,views_)
        {
            if(v != senderView)
                v->adjustZoom(r);
        }
    }
}

void LogRequestViewControlItem::adjustColumnWidth()
{
    if(model_)
    {
        for(int i=0; i < model_->columnCount()-1; i++)
            tree_->resizeColumnToContents(i);
    }
}

//=============================================
//
// LogRequestView
//
//=============================================

LogRequestView::LogRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    QScrollArea(parent),
    handler_(handler),
    data_(0),
    maxVal_(0),
    lastScanIndex_(0)
{
    Q_ASSERT(handler_);
    data_=handler_->data_;
    Q_ASSERT(data_);

    QWidget* holder=new QWidget(this);

    mainLayout_=new QHBoxLayout(holder);
    mainLayout_->setContentsMargins(0,0,0,0);
    mainLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);

    splitter_=new QSplitter(this);
    mainLayout_->addWidget(splitter_);

    //views
    QWidget* w=new QWidget(this);
    viewLayout_=new QVBoxLayout(w);
    viewLayout_->setContentsMargins(0,0,0,0);
    viewLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);
    splitter_->addWidget(w);

    //Sidebar
    w=new QWidget(this);
    sideLayout_=new QVBoxLayout(w);
    sideLayout_->setContentsMargins(0,0,0,0);
    sideLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);
    splitter_->addWidget(w);

    scanLabel_=new QLabel(this);
    sideLayout_->addWidget(scanLabel_);
    QColor bg(50,52,58);
    scanLabel_->setStyleSheet("QLabel{background: " + bg.name() + ";}");
    scanLabel_->setTextFormat(Qt::RichText);

    controlTab_ =new QTabWidget(this);
    controlTab_->setTabBarAutoHide(true);
    sideLayout_->addWidget(controlTab_);

    //init scrollarea
    setWidgetResizable(true);
    setWidget(holder);

    //initial splitter size
    QTimer::singleShot(1,this, SLOT(adjustSplitterSize()));
}

LogRequestView::~LogRequestView()
{
}

void LogRequestView::adjustSplitterSize()
{
    //initial splitter size
    QFont font;
    QFontMetrics fm(font);
    int hSize=fm.width("AAAAAverylongsuitename");
    QList<int> sizeLst=splitter_->sizes();
    if(sizeLst.count()==2)
    {
        static int usableWidth = 0;

        if(width() > hSize && usableWidth == 0)
            usableWidth = width();

        if(usableWidth > hSize)
        {
            sizeLst[1]=hSize;
            sizeLst[0]=usableWidth-hSize;
            splitter_->setSizes(sizeLst);
        }
    }
}

void LogRequestView::buildControlCore(LogRequestViewControlItem* item,QString title,QString modelHeader,bool addSelectAll)
{
    item->model_=new LogLoadRequestModel(modelHeader,this);
    item->sortModel_=new LogLoadRequestSortModel(this);
    item->sortModel_->setSourceModel(item->model_);
    item->sortModel_->setDynamicSortFilter(true);

    QWidget* w=new QWidget(this);
    QVBoxLayout* vb=new QVBoxLayout(w);
    vb->setContentsMargins(0,0,0,0);
    vb->setSpacing(1);

    item->tree_=new QTreeView(this);
    item->tree_->setRootIsDecorated(false);
    item->tree_->setAllColumnsShowFocus(true);
    item->tree_->setUniformRowHeights(true);
    item->tree_->setSortingEnabled(true);
    item->tree_->sortByColumn(1, Qt::DescendingOrder);
    item->tree_->setModel(item->sortModel_);
    vb->addWidget(item->tree_);

    controlTab_->addTab(w,title);

    QToolButton* unselectAllTb=new QToolButton(this);
    unselectAllTb->setText(tr("Unselect all"));
    QSizePolicy pol=unselectAllTb->sizePolicy();
    pol.setHorizontalPolicy(QSizePolicy::Expanding);
    unselectAllTb->setSizePolicy(pol);

    QToolButton* selectFourTb=new QToolButton(this);
    selectFourTb->setText(tr("Select 1-4"));
    selectFourTb->setSizePolicy(pol);

    if(addSelectAll)
    {
        QToolButton* selectAllTb=new QToolButton(this);
        selectAllTb->setText(tr("Select all"));
        selectAllTb->setSizePolicy(pol);
        vb->addWidget(selectAllTb);

        connect(selectAllTb,SIGNAL(clicked()),
            item->model_,SLOT(selectAll()));
    }

    vb->addWidget(selectFourTb);
    vb->addWidget(unselectAllTb);

    connect(unselectAllTb,SIGNAL(clicked()),
            item->model_,SLOT(unselectAll()));

    connect(selectFourTb,SIGNAL(clicked()),
            item->model_,SLOT(selectFirstFourItems()));
}

void LogRequestView::buildSuiteControl(LogRequestViewControlItem* item,QString title,QString modelHeader,bool addSelectAll)
{
    buildControlCore(item,title,modelHeader,addSelectAll);

    connect(item->model_,SIGNAL(checkStateChanged(int,bool)),
            this,SLOT(addRemoveSuite(int,bool)));

    connect(this,SIGNAL(suitePlotStateChanged(int,bool,QColor)),
            item->model_,SLOT(updateItem(int,bool,QColor)));
}

void LogRequestView::buildCmdControl(LogRequestViewControlItem* item,QString title,QString modelHeader,bool addSelectAll)
{
    buildControlCore(item,title,modelHeader,addSelectAll);

    connect(item->model_,SIGNAL(checkStateChanged(int,bool)),
            this,SLOT(addRemoveCmd(int,bool)));

    connect(this,SIGNAL(cmdPlotStateChanged(int,bool,QColor)),
            item->model_,SLOT(updateItem(int,bool,QColor)));
}

void LogRequestView::buildUidControl(LogRequestViewControlItem* item,QString title,QString modelHeader,bool addSelectAll)
{
    buildControlCore(item,title,modelHeader,addSelectAll);

    connect(item->model_,SIGNAL(checkStateChanged(int,bool)),
            this,SLOT(addRemoveUid(int,bool)));

    connect(this,SIGNAL(uidPlotStateChanged(int,bool,QColor)),
            item->model_,SLOT(updateItem(int,bool,QColor)));
}

QChart* LogRequestView::addChartById(QString id)
{
    QChart* chart = new QChart();
    ChartView* chartView=new ChartView(chart,this);
    chartView->setRenderHint(QPainter::Antialiasing);
    viewLayout_->addWidget(chartView);
    views_ << chartView;

    connect(chartView,SIGNAL(chartZoomed(QRectF)),
            this,SLOT(slotZoom(QRectF)));

    //connect(chartView,SIGNAL(chartZoomed(QRectF)),
    //        this,SIGNAL(zoomHappened(QRectF)));

    connect(chartView,SIGNAL(positionChanged(qreal)),
            this,SLOT(scanPositionChanged(qreal)));

    connect(chartView,SIGNAL(positionClicked(qreal)),
            this,SLOT(scanPositionClicked(qreal)));

    viewIds_[id]=chartView;
    return chart;
}

void LogRequestView::removeChartById(QString id)
{
    if(ChartView* chartView=viewIds_.value(id,NULL))
    {
        viewLayout_->removeWidget(chartView);
        delete chartView;
        views_.removeOne(chartView);
    }

}

QString LogRequestView::chartId(ChartView* cv)
{
    return viewIds_.key(cv,"");
}

void LogRequestView::slotZoom(QRectF r)
{
    if(ChartView* senderView=static_cast<ChartView*>(sender()))
    {
        Q_FOREACH(ChartView* v,views_)
        {
           if(v != senderView)
               v->doZoom(r);

           v->adjustCallout();
        }

        qint64 startTime, endTime;
        Q_ASSERT(!views_.isEmpty());
        views_[0]->currentTimeRange(startTime,endTime);

        size_t startIdx,endIdx;
        if(seriesPeriodIndex(startTime,endTime,startIdx,endIdx))
        {
            data_->computeStat(startIdx,endIdx);
            adjustStats();
        }

        //Notify the other request views handled by the handler
        Q_EMIT zoomHappened(r);
        Q_EMIT timeRangeChanged(startTime,endTime);
    }
}

void LogRequestView::adjustZoom(QRectF r)
{
    Q_FOREACH(ChartView* v,views_)
    {
        v->doZoom(r);
        v->adjustCallout();
    }

    adjustStats();
}

//Adjust the time range (zoom) of the last view using the time range
//in the first view
void LogRequestView::adjustZoom()
{
    qint64 start=-1, end=-1;
    if(views_.count() > 1)
    {
        views_[0]->currentTimeRange(start,end);

        if(start != -1 && end  !=-1)
        {
            ChartView* view=views_.back();
            view->doZoom(start,end);
            view->adjustCallout();
        }
   }
}

void LogRequestView::changeResolution()
{
    qint64 start=-1, end=-1;
    if(!views_.isEmpty())
    {
        views_[0]->currentTimeRange(start,end);
    }

    loadCore();
    loadSuites();

    if(start != -1 && end  !=-1)
    {
        Q_FOREACH(ChartView* v,views_)
        {
            v->doZoom(start,end);
            v->adjustCallout();
        }
    }
}

void LogRequestView::removeSeries(QChart* chart,QString id)
{
    Q_FOREACH(QAbstractSeries *s,chart->series())
    {
        if(s->name() == id)
        {
            chart->removeSeries(s);
            return;
        }
    }
}

QColor LogRequestView::seriesColour(QChart* chart,QString id)
{
    Q_FOREACH(QAbstractSeries *s,chart->series())
    {
        if(s->name().endsWith(id))
        {
            if(QLineSeries *ls=static_cast<QLineSeries*>(s))
                return ls->color();
            break;
        }
    }
    return QColor();
}

void LogRequestView::clear()
{   
    clearCharts();

    if(suiteCtl_.model_)
        suiteCtl_.model_->clearData();

    if(cmdCtl_.model_)
        cmdCtl_.model_->clearData();

    if(uidCtl_.model_)
        uidCtl_.model_->clearData();
}

void LogRequestView::clearCharts()
{
    Q_FOREACH(ChartView* v,views_)
    {
        Q_ASSERT(v->chart());
        v->chart()->removeAllSeries();

        //We do it in this way to get rid of error message:
        //"Can not remove axis. Axis not found on the chart."
        if(v->chart()->axes(Qt::Horizontal).contains(v->chart()->axisX()))
            v->chart()->removeAxis(v->chart()->axisX());

        if(v->chart()->axes(Qt::Vertical).contains(v->chart()->axisY()))
            v->chart()->removeAxis(v->chart()->axisY());
    }
    maxVal_=0;
}

void LogRequestView::clearViews()
{
    QLayoutItem* child=0;
    while ((child = viewLayout_->takeAt(0)) != 0)
    {
        QWidget* w=child->widget();
        delete child;
        if(w)
            delete w;
    }

    Q_ASSERT(viewLayout_->count() == 0);
    views_.clear();
    maxVal_=0;
}

void LogRequestView::load()
{
    suiteCtl_.plotState_.clear();
    for(size_t i=0; i < data_->suites().size(); i++)
        suiteCtl_.plotState_ << false;

    cmdCtl_.plotState_.clear();
    for(size_t i=0; i < data_->total().subReq().size(); i++)
        cmdCtl_.plotState_ << false;

    uidCtl_.plotState_.clear();
    for(size_t i=0; i < data_->uidData().size(); i++)
        uidCtl_.plotState_ << false;

    loadCore();
    loadSuites();

    suiteCtl_.adjustColumnWidth();
    cmdCtl_.adjustColumnWidth();
    uidCtl_.adjustColumnWidth();

    scanPositionChanged(0);
}

void LogRequestView::loadSuites()
{
    for(int i=0; i < suiteCtl_.plotState_.count(); i++)
    {
        if(suiteCtl_.plotState_[i])
        {
            addSuite(i);
        }
    }
}

void LogRequestView::build(ChartView* view,QLineSeries *series, QString title,int maxVal)
{
    Q_ASSERT(view);
    QChart *chart=view->chart();
    Q_ASSERT(chart);

    chart->addSeries(series);
    chart->setTitle(title);

    if(!chart->axisX())
    {
        chart->legend()->hide();
        QDateTimeAxis *axisX = new QDateTimeAxis;
        axisX->setTickCount(10);
        axisX->setFormat("HH dd/MM");
        chart->setAxisX(axisX, series);
        view->adjustTimeAxis(data_->period());

        QValueAxis *axisY = new QValueAxis;
        axisY->setLabelFormat("%i");

        QString yTitle;
        if(data_->timeRes() == LogLoadData::SecondResolution)
            yTitle="Req. per second";
        else if(data_->timeRes() == LogLoadData::MinuteResolution)
            yTitle="Req. per minute";
        else if(data_->timeRes() == LogLoadData::HourResolution)
            yTitle="Req. per hour";

        axisY->setTitleText(yTitle);
        axisY->setMin(0.);
        chart->setAxisY(axisY, series);
        axisY->setMin(0.);
        axisY->setMax(maxVal);
    }
    else
    {
        chart->addSeries(series);
        series->attachAxis(chart->axisX());
        series->attachAxis(chart->axisY());
    }
}

void LogRequestView::adjustMaxVal()
{
    Q_FOREACH(ChartView* v,views_)
    {
        Q_ASSERT(v->chart());
        if(QValueAxis *axisY=static_cast<QValueAxis*>(v->chart()->axisY()))
        {
            axisY->setMax(maxVal_);
        }
    }
}

void LogRequestView::showFullRange()
{
    Q_FOREACH(ChartView* view,views_)
    {
        view->chart()->zoomReset();
        view->adjustCallout();
    }

    adjustStats();

    Q_EMIT(timeRangeReset());
}

void LogRequestView::scanPositionClicked(qreal pos)
{
    if(pos < 1)
    {
        Q_FOREACH(ChartView* view,views_)
            view->removeCallout();

        Q_EMIT(timeRangeHighlighted(0,0,0));
    }
    else
    {
        qint64 t1(pos);
        qint64 t2=t1;

        qint64 tw=0;
        if(!views_.isEmpty())
            tw=views_[0]->widthToTimeRange(50.);

        //Try to find the nearest data point around the click position
        int idx=0;
        if(seriesIndex(t1,0,tw,idx))
        {
            QChart *chart=views_[0]->chart();

            QList<QAbstractSeries*> lst=chart->series();
            if(!lst.empty())
            {
                QLineSeries *ser=static_cast<QLineSeries*>(lst[0]);
                Q_ASSERT(ser);
                t1=ser->at(idx).x();
                t2=t1;
            }
        }

        if(data_->timeRes() == LogLoadData::MinuteResolution)
            t2=t1+60*1000;

        Q_FOREACH(ChartView* view,views_)
            view->setCallout(t1);

        Q_EMIT(timeRangeHighlighted(t1,t2,tw));
    }
}

bool LogRequestView::seriesPeriodIndex(qint64 startTime, qint64 endTime,size_t& startIdx,size_t& endIdx)
{
    startIdx=0;
    endIdx=0;
    int start=0,end=0;
    if(seriesIndex(startTime,0,0,start))
    {
        if(seriesIndex(endTime,startIdx,0,end))
        {
            startIdx=start;
            endIdx=end;
            return true;
        }
    }
    return false;
}

bool LogRequestView::seriesIndex(qint64 t,int startIdx,qint64 tolerance,int& idx)
{
    QChart *chart=views_[0]->chart();

    QList<QAbstractSeries*> lst=chart->series();
    if(lst.empty())
        return false;

    QLineSeries *ser=static_cast<QLineSeries*>(lst[0]);
    Q_ASSERT(ser);

    idx=-1;
    if(t < 0)
        return false;

    int num=ser->count();
    if(num == 0)
        return false;

    if(startIdx > num-1)
        startIdx=0;

    if(startIdx >= num)
        return false;

    if(t >= ser->at(startIdx).x())
    {
        if(tolerance <=0)
            tolerance=10*1000; //ms

        for(int i=startIdx; i < num; i++)
        {
            if(ser->at(i).x() >= t)
            {
                qint64 nextDelta=ser->at(i).x()-t;
                qint64 prevDelta=(i > 0)?(t-ser->at(i-1).x()):(nextDelta+1);
                if(prevDelta > nextDelta && nextDelta <=tolerance)
                {
                    idx=i;
                    return true;
                }
                else if(prevDelta < nextDelta && prevDelta <=tolerance)
                {
                    idx=i-1;
                    return true;
                }
                return false;
             }
        }
    }
    else
    {
        if(tolerance <=0)
            tolerance=10*1000; //ms

        for(int i=startIdx; i >=0; i--)
        {
            if(ser->at(i).x() <= t)
            {
                qint64 nextDelta=t-ser->at(i).x();
                qint64 prevDelta=(i < startIdx)?(ser->at(i+1).x()-t):(nextDelta+1);
                if(prevDelta > nextDelta && nextDelta <=tolerance)
                {
                    idx=i;
                    return true;
                }
                else if(prevDelta < nextDelta && prevDelta <=tolerance)
                {
                    idx=i+1;
                    return true;
                }
                return false;
            }
        }
    }

    return false;
}

qint64 LogRequestView::seriesTime(int idx)
{
    QChart *chart=views_[0]->chart();

    QList<QAbstractSeries*> lst=chart->series();
    if(lst.empty())
        return 0;

    QLineSeries *ser=static_cast<QLineSeries*>(lst[0]);
    Q_ASSERT(ser);
    return ser->at(idx).x();
}

int LogRequestView::seriesValue(QChart* chart,QString id,int idx)
{
    if(chart)
    {
        Q_FOREACH(QAbstractSeries *s,chart->series())
        {
            if(s->name().endsWith(id))
            {
                if(QLineSeries *ls=static_cast<QLineSeries*>(s))
                    return ls->at(idx).y();
                else
                    return 0;
            }
        }
    }

    return 0;
}

void LogRequestView::setScanText(QString txt)
{
    scanLabel_->setText(txt);
}

void LogRequestView::scanPositionChanged(qreal pos)
{
    qint64 t(pos);
    int idx=-1;

    if(views_.isEmpty())
    {
        QString t;
        setScanText(t);
        return;
    }

    qint64 tw=views_[0]->widthToTimeRange(50.);
    bool hasData=seriesIndex(t,lastScanIndex_,tw,idx);

    lastScanIndex_=idx;
    //UiLog().dbg() << "idx=" << idx;

    //QColor dateCol(210,212,218);
    QColor dateCol(210,211,214);
    //QString dateTxt=(hasData)?QDateTime::fromMSecsSinceEpoch(data_->time()[idx]).toString("hh:mm:ss dd/MM/yyyy"):" N/A";
    //QString txt=Viewer::formatText("date (): " + dateTxt, dateCol);

    QString txt="<table width=\'100%\' cellpadding=\'4px\'>";
    QString dateTxt=QDateTime::fromMSecsSinceEpoch(t).toString("hh:mm:ss dd/MM/yyyy");

    txt="<tr>" +
        Viewer::formatTableTdText("Date (cursor): ",dateCol) +
        Viewer::formatTableTdBg(dateTxt,dateCol) +
        "</tr>";

    //Viewer::formatText("date (at cursor): </td>" +
    //                QDateTime::fromMSecsSinceEpoch(t).toString("hh:mm:ss dd/MM/yyyy"),
    //                dateCol);

    dateTxt=(hasData)?QDateTime::fromMSecsSinceEpoch(seriesTime(idx)).toString("hh:mm:ss dd/MM/yyyy"):" N/A";
    txt+="<tr>" +
        Viewer::formatTableTdText("Date (nearest):",dateCol) +
        Viewer::formatTableTdBg(dateTxt,dateCol) +
        "</tr>";

    buildScanTable(txt,idx);

    setScanText(txt);
}

void LogRequestView::buildScanRow(QString &txt,QString name,size_t val,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(QString::number(val),numBg) + "</tr>";
}

void LogRequestView::buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(QString::number(tot),numBg) +
          Viewer::formatTableTdBg(QString::number(ch),numBg) +
          Viewer::formatTableTdBg(QString::number(us),numBg) + "</tr>";
}

void LogRequestView::buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(" N/A",numBg) +
          Viewer::formatTableTdBg(" N/A",numBg) +
          Viewer::formatTableTdBg(" N/A",numBg) + "</tr>";
}

void LogRequestView::buildEmptyScanRowSingleVal(QString &txt,QString name,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(" N/A",numBg) + "</tr>";
}


//=============================================================================
//
// LogTotalRequestView
//
//=============================================================================

LogTotalRequestView::LogTotalRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(viewLayout_);
    Q_ASSERT(sideLayout_);

    for(int i=0; i < 3; i++)
    {
        addChartById(QString::number(i));
    }

    UI_ASSERT(views_.count() == 3,"views_.count()=" << views_.count());

    buildSuiteControl(&suiteCtl_,tr("Suites"),tr("Suite"),false);
}

QChart* LogTotalRequestView::getChart(ChartType type)
{
    if(ChartView *v=getView(type))
        return v->chart();
    return 0;
}

ChartView* LogTotalRequestView::getView(ChartType type)
{
    UI_ASSERT(views_.count() == 3,"views_.count()=" << views_.count());
    switch(type)
    {
    case TotalChartType:
        return views_[0];
    case ChildChartType:
        return views_[1];
    case UserChartType:
        return views_[2];
    default:
        break;
    }

    return 0;
}

void LogTotalRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(data_->suites().size()))
    {
        suiteCtl_.plotState_[suiteIdx]=st;

        //Add suite
        if(st)
        {
            addSuite(suiteIdx);
            Q_EMIT suitePlotStateChanged(suiteIdx,true,
                   seriesColour(getChart(TotalChartType),suiteSeriesId(suiteIdx)));
        }
        //remove
        else
        {
            removeSeries(getChart(TotalChartType),
                              suiteSeriesId(suiteIdx));

            removeSeries(getChart(ChildChartType),
                              suiteSeriesId(suiteIdx));

            removeSeries(getChart(UserChartType),
                              suiteSeriesId(suiteIdx));
        }
    }
}

void LogTotalRequestView::addSuite(int idx)
{
    QChart* chart=0;

    QLineSeries* series=new QLineSeries();
    series->setName(suiteSeriesId(idx));
    data_->getSuiteTotalReq(idx,*series);
    chart=getChart(TotalChartType);
    chart->addSeries(series);
    series->attachAxis(chart->axisX());
    series->attachAxis(chart->axisY());

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName(suiteSeriesId(idx));
    data_->getSuiteChildReq(idx,*chSeries);
    chart=getChart(ChildChartType);
    chart->addSeries(chSeries);
    chSeries->attachAxis(chart->axisX());
    chSeries->attachAxis(chart->axisY());

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName(suiteSeriesId(idx));
    data_->getSuiteUserReq(idx,*usSeries);
    chart=getChart(UserChartType);
    chart->addSeries(usSeries);
    usSeries->attachAxis(chart->axisX());
    usSeries->attachAxis(chart->axisY());
}

void LogTotalRequestView::loadCore()
{
    Q_ASSERT(suiteCtl_.model_);
    suiteCtl_.model_->setData(data_->suites(),suiteCtl_.plotState_);

    clearCharts();

    int maxVal=0;
    QLineSeries* tSeries=new QLineSeries();
    tSeries->setName("all");
    data_->getTotalReq(*tSeries,maxVal);

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("all");
    data_->getChildReq(*chSeries);

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("all");
    data_->getUserReq(*usSeries);

    build(getView(TotalChartType),tSeries,"Child+User requests",maxVal);
    build(getView(ChildChartType),chSeries,"Child requests",maxVal);
    build(getView(UserChartType),usSeries,"User requests",maxVal);
}

void LogTotalRequestView::adjustStats()
{
    suiteCtl_.model_->adjustStats(data_->suites());
}

QString LogTotalRequestView::suiteSeriesId(int idx) const
{
    return "s_" + QString::number(idx);
}


void LogTotalRequestView::buildScanTable(QString& txt,int idx)
{
    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Item",hdrCol) +
          Viewer::formatTableThText("Total",hdrCol) +
          Viewer::formatTableThText("Child",hdrCol) +
          Viewer::formatTableThText("User",hdrCol) + "</tr>";

    QChart* tChart=getChart(TotalChartType);
    QChart* cChart=getChart(ChildChartType);
    QChart* uChart=getChart(UserChartType);

    if(idx != -1)
    {
        size_t tot=0,ch=0,us=0;
        QColor col=seriesColour(tChart,"all");
        QString name="all";

        tot=seriesValue(tChart,"all",idx);
        ch=seriesValue(cChart,"all",idx);
        us=seriesValue(uChart,"all",idx);
        buildScanRow(txt,name,tot,ch,us,col);

        for(int i=0; i < suiteCtl_.plotState_.size(); i++)
        {
            if(suiteCtl_.plotState_[i])
            {
                tot=0;ch=0;us=0;
                QString id=suiteSeriesId(i);
                col=seriesColour(tChart,id);
                name=QString::fromStdString(data_->suites()[i].name());
                tot=seriesValue(tChart,id,idx);
                ch=seriesValue(cChart,id,idx);
                us=seriesValue(uChart,id,idx);
                buildScanRow(txt,name,tot,ch,us,col);
            }
        }
    }
    else
    {
        QColor col=seriesColour(tChart,"all");
        QString name="all";
        buildEmptyScanRow(txt,name,col);
        for(int i=0; i < suiteCtl_.plotState_.size(); i++)
        {
            if(suiteCtl_.plotState_[i])
            {
                QString id=suiteSeriesId(i);
                col=seriesColour(tChart,id);
                name=QString::fromStdString(data_->suites()[i].name());
                buildEmptyScanRow(txt,name,col);
            }
        }
    }

    txt+="</table>";
}

//=============================================================================
//
// LogCmdSuiteRequestView
//
//=============================================================================

LogCmdSuiteRequestView::LogCmdSuiteRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    buildSuiteControl(&suiteCtl_,tr("Suites"),tr("Suite"),false);
    buildCmdControl(&cmdCtl_,tr("Commands"),tr("Command"),false);

    controlTab_->setCurrentIndex(2);
}

//One chart = one suite with all the subrequests (child + user)
void LogCmdSuiteRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(data_->suites().size()))
    {
        suiteCtl_.plotState_[suiteIdx]=st;

        //Add suite
        if(st)
        {
            addSuite(suiteIdx);
            QString id=QString::number(suiteIdx);
            ChartView* view=viewIds_.value(id,NULL);
            Q_ASSERT(view);
            //It only woks if the chart is already displayed, so we need a delayed
            //adjustment
            QTimer::singleShot(0,this, SLOT(adjustZoom()));
        }
        //remove
        else
        {
            removeChartById(QString::number(suiteIdx));
        }
    }
}

void LogCmdSuiteRequestView::addSuite(int suiteIdx)
{
    //at this point total must be already added, so we do not need to adjust the
    //maxval
    QString id=QString::number(suiteIdx);
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="suite: " +
            QString::fromStdString(data_->suites()[suiteIdx].name());

    for(int i=0; i < cmdCtl_.plotState_.count(); i++)
    {
        if(cmdCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(cmdSeriesId(i));

            data_->getSuiteSubReq(suiteIdx,i,*series);
            build(view,series,title,maxVal_);
        }
    }
}

void LogCmdSuiteRequestView::addTotal()
{
    int prevMaxVal=maxVal_;

    QString id="total";
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="All suites";

    for(int i=0; i < cmdCtl_.plotState_.count(); i++)
    {
        if(cmdCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(cmdSeriesId(i));

            int maxVal=0;
            data_->getSubReq(i,*series,maxVal);
            if(maxVal_< maxVal)
                maxVal_=maxVal;

            build(view,series,title,maxVal_);

            Q_EMIT cmdPlotStateChanged(i,true,
                    cmdSeriesColour(view->chart(),i));
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}


//One chart = one suite
void LogCmdSuiteRequestView::addRemoveCmd(int reqIdx, bool st)
{
    //if(childReqIdx >= 0 && childReqIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        cmdCtl_.plotState_[reqIdx]=st;

        //Add suite
        if(st)
        {
            addCmd(reqIdx);
            if(views_.count() >0)
            {
                Q_EMIT cmdPlotStateChanged(reqIdx,true,
                   cmdSeriesColour(views_[0]->chart(),reqIdx));
            }
        }
        //remove
        else
        {
            removeCmd(reqIdx);
        }
    }
}

void LogCmdSuiteRequestView::addCmd(int reqIdx)
{
    int prevMaxVal=maxVal_;

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);

        if(chartId(views_[i]) == "total")
        {
            QString title="All suites";

            QLineSeries* series=new QLineSeries();
            series->setName(cmdSeriesId(reqIdx));

            int maxVal=0;
            data_->getSubReq(reqIdx,*series,maxVal);
            if(maxVal > maxVal_)
                maxVal_=maxVal;

            build(views_[i],series,title,maxVal_);
        }
        else
        {
            int suiteIdx=chartId(views_[i]).toInt();
            Q_ASSERT(suiteIdx >= 0);

            QString title="suite: " +
                QString::fromStdString(data_->suites()[suiteIdx].name());

            QLineSeries* series=new QLineSeries();
            series->setName(cmdSeriesId(reqIdx));
            data_->getSuiteSubReq(suiteIdx,reqIdx,*series);

            build(views_[i],series,title,maxVal_);
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

void LogCmdSuiteRequestView::removeCmd(int reqIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        removeSeries(views_[i]->chart(),cmdSeriesId(reqIdx));
    }
}

QString LogCmdSuiteRequestView::cmdSeriesId(int idx) const
{
    return "c_" + QString::number(idx);
}

//The command colour is the same for all the suites
QColor LogCmdSuiteRequestView::cmdSeriesColour(QChart* chart,size_t reqIdx)
{
    return seriesColour(chart,cmdSeriesId(reqIdx));
}

void LogCmdSuiteRequestView::loadCore()
{
    Q_ASSERT(suiteCtl_.model_);
    suiteCtl_.model_->setData(data_->suites(),suiteCtl_.plotState_);

    Q_ASSERT(cmdCtl_.model_);
    cmdCtl_.model_->setData(data_->total().subReq(),cmdCtl_.plotState_);

    //Removes everything
    clearViews();

    //Total
    addTotal();

    if(!cmdCtl_.isAnySet())
        cmdCtl_.model_->selectFirstItem();
}

void LogCmdSuiteRequestView::buildScanTable(QString& txt,int idx)
{
    if(views_.count() == 0)
        return;

    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Command",hdrCol) +
        Viewer::formatTableThText("Request",hdrCol) +
        "</tr>";

    for(int i=0; i < views_.count(); i++)
    {
        QChart *chart=views_[i]->chart();
        Q_ASSERT(chart);
        QString id=chartId(views_[i]);
        QString name;
        if(id == "total")
        {
            name="all suites";
        }
        else
        {
            int suiteIdx=chartId(views_[i]).toInt();
            Q_ASSERT(suiteIdx >= 0);
            name="suite: " + data_->suiteNames()[suiteIdx];
        }

        txt+="<tr><td colspan=\'2\' bgcolor=\'" + QColor(140,140,140).name()  + "\'>" +
                Viewer::formatText(name,QColor(230,230,230)) + "</td></tr>";


        for(int j=0; j < cmdCtl_.plotState_.count(); j++)
        {
            if(cmdCtl_.plotState_[j])
            {
                QString id=cmdSeriesId(j);
                QColor col=seriesColour(chart,id);

                if(idx != -1)
                {
                    int val=seriesValue(chart,id,idx);
                    buildScanRow(txt,data_->subReqName(j),val,col);
                }
                else
                {
                    buildEmptyScanRowSingleVal(txt,data_->subReqName(j),col);
                }
            }
        }
    }

    txt+="</table>";
}

//=============================================================================
//
// LogSuiteCmdRequestView
//
//=============================================================================

LogSuiteCmdRequestView::LogSuiteCmdRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    buildSuiteControl(&suiteCtl_,tr("Suites"),tr("Suite"),false);
    buildCmdControl(&cmdCtl_,tr("Commands"),tr("Command"),false);

    controlTab_->setCurrentIndex(0);
}

//One chart = one suite
void LogSuiteCmdRequestView::addRemoveCmd(int reqIdx, bool st)
{
    cmdCtl_.plotState_[reqIdx]=st;

    //Add
    if(st)
    {
        addCmd(reqIdx);
        ChartView* view=viewIds_.value(cmdChartId(reqIdx),NULL);
        Q_ASSERT(view);
        //It only woks if the chart is already displayed, so we need a delayed
        //adjustment
        QTimer::singleShot(0,this, SLOT(adjustZoom()));
    }
    //remove
    else
    {
        removeChartById(cmdChartId(reqIdx));
    }
}

void LogSuiteCmdRequestView::addCmd(int reqIdx)
{
    //at this point total must be already added, so we do not need to adjust the
    //maxval
    QString id=cmdChartId(reqIdx);
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="cmd: " + data_->subReqName(reqIdx);

    for(int i=0; i < suiteCtl_.plotState_.count(); i++)
    {
        if(suiteCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(suiteSeriesId(i));

            data_->getSuiteSubReq(i,reqIdx,*series);
            build(view,series,title,maxVal_);
        }
    }
}

void LogSuiteCmdRequestView::addTotal()
{
    int prevMaxVal=maxVal_;

    QString id="total";
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="All commands";

    for(int i=0; i < suiteCtl_.plotState_.count(); i++)
    {
        if(suiteCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(suiteSeriesId(i));

            int maxVal=0;
            data_->getSuiteTotalReq(i,*series);
            if(maxVal_< maxVal)
                maxVal_=maxVal;

            build(view,series,title,maxVal_);

            Q_EMIT suitePlotStateChanged(i,true,
               suiteSeriesColour(view->chart(),i));
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}


//One chart = one suite
void LogSuiteCmdRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    suiteCtl_.plotState_[suiteIdx]=st;

    //Add suite
    if(st)
    {
        addSuite(suiteIdx);
        if(views_.count() >0)
        {
                Q_EMIT suitePlotStateChanged(suiteIdx,true,
                   suiteSeriesColour(views_[0]->chart(),suiteIdx));
        }
        //remove
        else
        {
            removeSuite(suiteIdx);
        }
    }
}

void LogSuiteCmdRequestView::addSuite(int suiteIdx)
{
    int prevMaxVal=maxVal_;

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);

        if(chartId(views_[i]) == "total")
        {
            QString title="All commands";

            QLineSeries* series=new QLineSeries();
            series->setName(suiteSeriesId(suiteIdx));

            int maxVal=0;
            data_->getSuiteTotalReq(suiteIdx,*series);
            if(maxVal > maxVal_)
                maxVal_=maxVal;

            build(views_[i],series,title,maxVal_);
        }
        else
        {
            int cmdIdx=chartId(views_[i]).toInt();
            Q_ASSERT(cmdIdx >= 0);

            QString title="cmd: " + data_->subReqName(cmdIdx);

            QLineSeries* series=new QLineSeries();
            series->setName(suiteSeriesId(suiteIdx));           
            data_->getSuiteSubReq(suiteIdx,cmdIdx,*series);
            build(views_[i],series,title,maxVal_);
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

void LogSuiteCmdRequestView::removeSuite(int suiteIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        removeSeries(views_[i]->chart(),suiteSeriesId(suiteIdx));
    }
}

QString LogSuiteCmdRequestView::cmdChartId(int idx) const
{
    return QString::number(idx);
}

QString LogSuiteCmdRequestView::suiteSeriesId(int suiteIdx) const
{
    return QString::number(suiteIdx);
}

QColor LogSuiteCmdRequestView::suiteSeriesColour(QChart* chart,size_t suiteIdx)
{
    return seriesColour(chart,suiteSeriesId(suiteIdx));
}

void LogSuiteCmdRequestView::loadCore()
{
    Q_ASSERT(suiteCtl_.model_);
    suiteCtl_.model_->setData(data_->suites(),suiteCtl_.plotState_);

    Q_ASSERT(cmdCtl_.model_);
    cmdCtl_.model_->setData(data_->total().subReq(),cmdCtl_.plotState_);

    //Removes everything
    clearViews();

    //Total
    addTotal();

    if(!suiteCtl_.isAnySet())
        suiteCtl_.model_->selectFirstItem();
}

void LogSuiteCmdRequestView::buildScanTable(QString& txt,int idx)
{
    if(views_.count() == 0)
        return;

    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Suite",hdrCol) +
        Viewer::formatTableThText("Request",hdrCol) +
        "</tr>";

    for(int i=0; i < views_.count(); i++)
    {
        QChart *chart=views_[i]->chart();
        Q_ASSERT(chart);
        QString id=chartId(views_[i]);
        QString name;
        int cmdIdx=id.toInt();


        if(id == "total")
        {
            name="all commands";
        }
        else
        {           
            Q_ASSERT(cmdIdx >= 0);          
            name="cmd: " + data_->subReqName(cmdIdx);
        }

        txt+="<tr><td colspan=\'2\' bgcolor=\'" + QColor(140,140,140).name()  + "\'>" +
                Viewer::formatText(name,QColor(230,230,230)) + "</td></tr>";


        for(int j=0; j < suiteCtl_.plotState_.count(); j++)
        {
            if(suiteCtl_.plotState_[j])
            {
                QString id=suiteSeriesId(j);
                QColor col=seriesColour(chart,id);

                if(idx != -1)
                {
                    int val=seriesValue(chart,id,idx);
                    buildScanRow(txt,data_->suiteNames()[j],val,col);
                }
                else
                {
                    buildEmptyScanRowSingleVal(txt,data_->suiteNames()[j],col);
                }
            }
        }
    }

    txt+="</table>";
}

//=============================================================================
//
// LogUidCmdRequestView
//
// One chart = show uid activity graphs for the given command
//=============================================================================

LogUidCmdRequestView::LogUidCmdRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    buildUidControl(&uidCtl_,tr("Users"),tr("User"),false);
    buildCmdControl(&cmdCtl_,tr("Commands"),tr("Command"),false);

    controlTab_->setCurrentIndex(0);
}

//One chart = all the users for the given subrequest/command
void LogUidCmdRequestView::addRemoveUid(int uidIdx, bool st)
{
    if(uidIdx >= 0 && uidIdx < static_cast<int>(data_->uidData().size()))
    {
        uidCtl_.plotState_[uidIdx]=st;

        //Add uid
        if(st)
        {
            addUid(uidIdx);
            if(views_.size() >0)
            {
                Q_EMIT uidPlotStateChanged(uidIdx,true,
                   seriesColour(views_[0]->chart(),uidSeriesId(uidIdx)));
            }
        }
        //remove
        else
        {
            removeUid(uidIdx);
        }
    }
}
void LogUidCmdRequestView::addUid(int uidIdx)
{
    int prevMaxVal=maxVal_;

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);

        if(chartId(views_[i]) == "total")
        {
            QString title="All commands";

            QLineSeries* series=new QLineSeries();
            series->setName(uidSeriesId(uidIdx));

            int maxVal=0;
            data_->getUidTotalReq(uidIdx,*series,maxVal);
            if(maxVal > maxVal_)
                maxVal_=maxVal;

            build(views_[i],series,title,maxVal_);
        }
        else
        {
            QString id=chartId(views_[i]);
            int idx=id.toInt();
            Q_ASSERT(idx >= 0);
            QString title="command: " + data_->subReqName(idx);

            QLineSeries* series=new QLineSeries();

            int userReqIdx=idx;
            series->setName(uidSeriesId(uidIdx));
            data_->getUidSubReq(uidIdx,userReqIdx,*series);

            build(views_[i],series,title,data_->subReqMax());
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

void LogUidCmdRequestView::removeUid(int uidIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        removeSeries(views_[i]->chart(),uidSeriesId(uidIdx));
    }
}

//One chart = one scommand
void LogUidCmdRequestView::addRemoveCmd(int reqIdx, bool st)
{
    cmdCtl_.plotState_[reqIdx]=st;

    if(st)
    {
        addCmd(reqIdx);
        QString id=QString::number(reqIdx);
        ChartView* view=viewIds_.value(id,NULL);
        Q_ASSERT(view);
        //It only woks if the chart is already displayed, so we need a delayed
        //adjustment
        QTimer::singleShot(0,this, SLOT(adjustZoom()));
    }
    else
    {
        removeChartById(cmdChartId(reqIdx));
    }
}


void LogUidCmdRequestView::addCmd(int reqIdx)
{
    QString id=cmdChartId(reqIdx);
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="command: " + data_->subReqName(reqIdx);

    for(int i=0; i < uidCtl_.plotState_.count(); i++)
    {
        if(uidCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(uidSeriesId(i));
            data_->getUidSubReq(i,reqIdx,*series);
            build(view,series,title,maxVal_);
        }
    }
}

void LogUidCmdRequestView::addTotal()
{
    int prevMaxVal=maxVal_;

    QString id="total";
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="All commands";

    for(int i=0; i < uidCtl_.plotState_.count(); i++)
    {
        if(uidCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(uidSeriesId(i));
            int maxVal=0;
            data_->getUidTotalReq(i,*series,maxVal);
            if(maxVal_< maxVal)
                maxVal_=maxVal;

            build(view,series,title,maxVal);

            Q_EMIT uidPlotStateChanged(i,true,
               uidSeriesColour(view->chart(),i));
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

QString LogUidCmdRequestView::uidSeriesId(int uidIdx) const
{
    return "u_" + QString::number(uidIdx);
}

QColor LogUidCmdRequestView::uidSeriesColour(QChart* chart,int uidIdx)
{
    return seriesColour(chart,uidSeriesId(uidIdx));
}

QString LogUidCmdRequestView::cmdChartId(int idx) const
{
    return QString::number(idx);
}

void LogUidCmdRequestView::loadCore()
{
    Q_ASSERT(uidCtl_.model_);
    uidCtl_.model_->setData(data_->uidData(),uidCtl_.plotState_);

    Q_ASSERT(cmdCtl_.model_);
    cmdCtl_.model_->setData(data_->total().subReq(),cmdCtl_.plotState_);

    //Removes everything
    clearViews();

    //Total
    addTotal();

    if(!uidCtl_.isAnySet())
        uidCtl_.model_->selectFirstItem();
}

void LogUidCmdRequestView::buildScanTable(QString& txt,int idx)
{
    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Command",hdrCol) +
        Viewer::formatTableThText("Request",hdrCol) +
        "</tr>";

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);
        QChart *chart=views_[i]->chart();
        Q_ASSERT(chart);
        QString id=chartId(views_[i]);

        QString cmd;
        if(id == "total")
        {
            cmd="all";
        }
        else
        {
            int cmdIdx=id.toInt();
            Q_ASSERT(cmdIdx >= 0);
            cmd=data_->subReqName(cmdIdx);
        }

        txt+="<tr><td colspan=\'2\' bgcolor=\'" + QColor(140,140,140).name()  + "\'>" +
                Viewer::formatText("command: " + cmd,QColor(230,230,230)) +
                "</td></tr>";

        for(int j=0; j < uidCtl_.plotState_.count(); j++)
        {
            if(uidCtl_.plotState_[j])
            {
                QString id=uidSeriesId(j);
                QColor col=seriesColour(chart,id);

                if(idx != -1)
                {
                    int val=seriesValue(chart,id,idx);
                    buildScanRow(txt,data_->uidName(j),val,col);
                }
                else
                {
                    buildEmptyScanRowSingleVal(txt,data_->uidName(j),col);
                }
            }
        }
    }

    txt+="</table>";
}

//=============================================================================
//
// LogCmdUidRequestView
//
//=============================================================================

LogCmdUidRequestView::LogCmdUidRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    buildUidControl(&uidCtl_,tr("Users"),tr("User"),false);
    buildCmdControl(&cmdCtl_,tr("Commands"),tr("Command"),false);

    controlTab_->setCurrentIndex(1);
}

//One chart = all the commands for the given uid
void LogCmdUidRequestView::addRemoveCmd(int reqIdx, bool st)
{
    if(reqIdx >= 0 && reqIdx < static_cast<int>(data_->total().subReq().size()))
    {
        cmdCtl_.plotState_[reqIdx]=st;

        //Add uid
        if(st)
        {
            addCmd(reqIdx);
            if(views_.size() >0)
            {
                Q_EMIT cmdPlotStateChanged(reqIdx,true,
                   seriesColour(views_[0]->chart(),cmdSeriesId(reqIdx)));
            }
        }
        //remove
        else
        {
            removeCmd(reqIdx);
        }
    }
}
void LogCmdUidRequestView::addCmd(int reqIdx)
{
    int prevMaxVal=maxVal_;

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);

        if(chartId(views_[i]) == "total")
        {
            QString title="All uids";

            QLineSeries* series=new QLineSeries();
            series->setName(cmdSeriesId(reqIdx));

            int maxVal=0;
            data_->getUidTotalReq(i,*series,maxVal);
            if(maxVal > maxVal_)
                maxVal_=maxVal;

            build(views_[i],series,title,maxVal_);
        }
        else
        {
            QString id=chartId(views_[i]);
            int idx=id.toInt();
            Q_ASSERT(idx >= 0);
            QString title="uid: " + data_->uidName(idx);

            QLineSeries* series=new QLineSeries();

            int uidIdx=idx;
            series->setName(cmdSeriesId(reqIdx));
            data_->getUidSubReq(uidIdx,reqIdx,*series);
            build(views_[i],series,title,data_->subReqMax());
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

void LogCmdUidRequestView::removeCmd(int reqIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        removeSeries(views_[i]->chart(),cmdSeriesId(reqIdx));
    }
}

//One chart = one user
void LogCmdUidRequestView::addRemoveUid(int uidIdx, bool st)
{
    uidCtl_.plotState_[uidIdx]=st;

    if(st)
    {
        addUid(uidIdx);
        QString id=QString::number(uidIdx);
        ChartView* view=viewIds_.value(id,NULL);
        Q_ASSERT(view);
        //It only woks if the chart is already displayed, so we need a delayed
        //adjustment
        QTimer::singleShot(0,this, SLOT(adjustZoom()));
    }
    else
    {
        removeChartById(uidChartId(uidIdx));
    }
}

void LogCmdUidRequestView::addUid(int uidIdx)
{
    QString id=uidChartId(uidIdx);
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="user: " + data_->uidName(uidIdx);

    for(int i=0; i < cmdCtl_.plotState_.count(); i++)
    {
        if(cmdCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(cmdSeriesId(i));
            data_->getUidSubReq(uidIdx,i,*series);
            build(view,series,title,data_->subReqMax());
        }
    }
}

void LogCmdUidRequestView::addTotal()
{
    int prevMaxVal=maxVal_;

    QString id="total";
    addChartById(id);
    ChartView* view=viewIds_.value(id,NULL);
    Q_ASSERT(view);

    QString title="All users";

    for(int i=0; i < cmdCtl_.plotState_.count(); i++)
    {
        if(cmdCtl_.plotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName(cmdSeriesId(i));
            int maxVal=0;
            data_->getSubReq(i,*series,maxVal);
            if(maxVal_< maxVal)
                maxVal_=maxVal;

            build(view,series,title,maxVal);

            Q_EMIT cmdPlotStateChanged(i,true,
               cmdSeriesColour(view->chart(),i));
        }
    }

    if(maxVal_ > prevMaxVal)
    {
        adjustMaxVal();
    }
}

QString LogCmdUidRequestView::cmdSeriesId(int reqIdx) const
{
    return "c_" + QString::number(reqIdx);
}

QColor LogCmdUidRequestView::cmdSeriesColour(QChart* chart,int reqIdx)
{
    return seriesColour(chart,cmdSeriesId(reqIdx));
}

QString LogCmdUidRequestView::uidChartId(int idx) const
{
    return QString::number(idx);
}


void LogCmdUidRequestView::loadCore()
{
    Q_ASSERT(uidCtl_.model_);
    uidCtl_.model_->setData(data_->uidData(),uidCtl_.plotState_);

    Q_ASSERT(cmdCtl_.model_);
    cmdCtl_.model_->setData(data_->total().subReq(),cmdCtl_.plotState_);

    //Removes everything
    clearViews();

    //Total
    addTotal();

    if(!cmdCtl_.isAnySet())
        cmdCtl_.model_->selectFirstItem();
}

void LogCmdUidRequestView::buildScanTable(QString& txt,int idx)
{
    return;
#if 0
    txt+="</table>";
    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Suite",hdrCol) +
        Viewer::formatTableThText("Request",hdrCol) +
        "</tr>";

    for(int i=0; i < views_.count(); i++)
    {
        Q_ASSERT(views_[i]);
        QChart *chart=views_[i]->chart();
        Q_ASSERT(chart);
        QString id=chartId(views_[i]);
        QString type;
        int cmdIdx=-1;
        parseChartId(id,type,cmdIdx);
        Q_ASSERT(cmdIdx >= 0);

        QString cmd;
        if(type == "c")
        {
            cmd=handler_->data_->childSubReqName(cmdIdx);
        }
        else if(type == "u")
        {
            cmd=handler_->data_->userSubReqName(cmdIdx);
        }

        txt+="<tr><td colspan=\'2\' bgcolor=\'" + QColor(140,140,140).name()  + "\'>" +
                Viewer::formatText("command: " + cmd,QColor(230,230,230)) +
                "</td></tr>";

        //txt+="</table>";
        //txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";

        if(idx != -1)
        {
            for(int j=0; j < handler_->suitePlotState().count(); j++)
            {
                if(handler_->suitePlotState()[j])
                {
                    QString id=suiteSeriesId(j);
                    QColor col=seriesColour(chart,id);
                    int val=seriesValue(chart,id,idx);
                    buildScanRow(txt,handler_->data_->suiteNames()[j],val,col);
                }
            }
        }
        else
        {

        }
    }

    txt+="</table>";
 #endif
}

//=============================================================================
//
// LogStatRequestView
//
//=============================================================================

LogStatRequestView::LogStatRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    statModel_=new LogStatRequestModel(this);
    statSortModel_=new QSortFilterProxyModel(this);
    statSortModel_->setSourceModel(statModel_);
    statSortModel_->setSortRole(Qt::DisplayRole);
    statSortModel_->setDynamicSortFilter(true);

    statTable_=new QTableView(this);
    statTable_->setSortingEnabled(true);
    statTable_->sortByColumn(1, Qt::DescendingOrder);
    statTable_->setModel(statSortModel_);

    viewLayout_->addWidget(statTable_);

    scanLabel_->hide();
}

void LogStatRequestView::adjustZoom(QRectF r)
{
    adjustStats();
}

//=============================================================================
//
// LogStatUidCmdView
//
//=============================================================================

LogStatCmdUidView::LogStatCmdUidView(LogRequestViewHandler* handler,QWidget* parent) :
    LogStatRequestView(handler,parent)
{
    buildUidControl(&uidCtl_,tr("Users"),tr("User"),true);
}

void LogStatCmdUidView::addRemoveUid(int uidIdx,bool st)
{
    for(int i=0; i < statModel_->columnCount(); i++)
    {
        if(statModel_->dataIndex(i) == uidIdx)
            statTable_->setColumnHidden(i,!st);
    }
}

void LogStatCmdUidView::adjustStats()
{
    uidCtl_.model_->adjustStats(data_->uidData());
    statModel_->setDataCmdUid(data_->total(),data_->uidData());
}

void LogStatCmdUidView::loadCore()
{
    Q_ASSERT(uidCtl_.model_);
    uidCtl_.model_->setData(data_->uidData(),uidCtl_.plotState_);

    uidCtl_.model_->selectAll();

    statModel_->setDataCmdUid(data_->total(),data_->uidData());

    for(int i=0; i < statModel_->columnCount()-1; i++)
        statTable_->resizeColumnToContents(i);
}

//=============================================================================
//
// LogStatUidCmdView
//
//=============================================================================

LogStatUidCmdView::LogStatUidCmdView(LogRequestViewHandler* handler,QWidget* parent) :
    LogStatRequestView(handler,parent)
{
    buildCmdControl(&cmdCtl_,tr("Commands"),tr("Command"),true);
}

void LogStatUidCmdView::addRemoveCmd(int reqIdx,bool st)
{
    for(int i=0; i < statModel_->columnCount(); i++)
    {
        if(statModel_->dataIndex(i) == reqIdx)
            statTable_->setColumnHidden(i,!st);
    }
}

void LogStatUidCmdView::adjustStats()
{
    cmdCtl_.model_->adjustStats(data_->total().subReq());
    statModel_->setDataUidCmd(data_->total(),data_->uidData());
}

void LogStatUidCmdView::loadCore()
{
    Q_ASSERT(cmdCtl_.model_);
    cmdCtl_.model_->setData(data_->total().subReq(),cmdCtl_.plotState_);
    cmdCtl_.model_->selectAll();

    statModel_->setDataUidCmd(data_->total(),data_->uidData());

    for(int i=0; i < statModel_->columnCount()-1; i++)
        statTable_->resizeColumnToContents(i);
}

//=============================================================================
//
// LogStatCmdSuiteView
//
//=============================================================================

LogStatCmdSuiteView::LogStatCmdSuiteView(LogRequestViewHandler* handler,QWidget* parent) :
    LogStatRequestView(handler,parent)
{
    buildSuiteControl(&suiteCtl_,tr("Suites"),tr("Suites"),true);
}

void LogStatCmdSuiteView::addRemoveSuite(int suiteIdx,bool st)
{
    for(int i=0; i < statModel_->columnCount(); i++)
    {
        if(statModel_->dataIndex(i) == suiteIdx)
            statTable_->setColumnHidden(i,!st);
    }
}

void LogStatCmdSuiteView::adjustStats()
{
    suiteCtl_.model_->adjustStats(data_->suiteData());
    statModel_->setDataCmdSuite(data_->total(),data_->suiteData());
}

void LogStatCmdSuiteView::loadCore()
{
    Q_ASSERT(suiteCtl_.model_);
    suiteCtl_.model_->setData(data_->suiteData(),suiteCtl_.plotState_);

    suiteCtl_.model_->selectAll();

    statModel_->setDataCmdSuite(data_->total(),data_->suiteData());

    for(int i=0; i < statModel_->columnCount()-1; i++)
        statTable_->resizeColumnToContents(i);
}

//=============================================================================
//
// LogStatSuiteCmdView
//
//=============================================================================

LogStatSuiteCmdView::LogStatSuiteCmdView(LogRequestViewHandler* handler,QWidget* parent) :
    LogStatRequestView(handler,parent)
{
    buildCmdControl(&cmdCtl_,tr("Commands"),tr("Command"),true);
}

void LogStatSuiteCmdView::addRemoveCmd(int idx, bool st)
{
    for(int i=0; i < statModel_->columnCount(); i++)
    {
        if(statModel_->dataIndex(i) == idx)
            statTable_->setColumnHidden(i,!st);
    }
}

void LogStatSuiteCmdView::adjustStats()
{
    cmdCtl_.model_->adjustStats(data_->total().subReq());
    statModel_->setDataSuiteCmd(data_->total(),data_->suiteData());
}

void LogStatSuiteCmdView::loadCore()
{
    Q_ASSERT(cmdCtl_.model_);
    cmdCtl_.model_->setData(data_->total().subReq(),cmdCtl_.plotState_);
    cmdCtl_.model_->selectAll();

    statModel_->setDataSuiteCmd(data_->total(),data_->suiteData());

    for(int i=0; i < statModel_->columnCount()-1; i++)
        statTable_->resizeColumnToContents(i);
}


//=====================================================
//
//  LogStatRequestModel
//
//=====================================================

LogStatRequestModel::LogStatRequestModel(QObject *parent) :
          QAbstractItemModel(parent), columnOrder_(ValueOrder)
{
}

LogStatRequestModel::~LogStatRequestModel()
{
}

void LogStatRequestModel::setData(const std::vector<LogRequestItem>& data)
{
    beginResetModel();

    endResetModel();
}


void LogStatRequestModel::setDataCmdUid(const LogLoadDataItem& total, const std::vector<LogLoadDataItem>& data)
{
    beginResetModel();

    data_.clear();

    data_.colLabels_ << "ALL";
    for(size_t i=0; i < data.size(); i++)
        data_.colLabels_ << "";

    data_.vals_=QVector<QVector<float> >(1+data.size());
    data_.dataIndex_=QVector<int>(1+data.size(),-1);

    QVector<float> val;
    for(size_t i=0; i < total.subReq().size(); i++)
    {
        val << total.subReq()[i].periodStat().sumTotal_;
        data_.rowLabels_ << QString::fromStdString(total.subReq()[i].name_);
    }  
    data_.vals_[0] =val;
    data_.dataIndex_[0]=-1;

    for(size_t i=0; i < data.size(); i++)
    {
        size_t pos=0;
        if(columnOrder_ == NameOrder)
            pos=i+1;
        else
            pos=data[i].rank()+1;

        data_.dataIndex_[pos]=i;
        data_.colLabels_[pos] = QString::fromStdString(data[i].name());

        val.clear();
        for(size_t j=0; j < data[i].subReq().size(); j++)
        {
            val << data[i].subReq()[j].periodStat().sumTotal_;
        }

        data_.vals_[pos] = val;
    }

    endResetModel();
}

void LogStatRequestModel::setDataUidCmd(const LogLoadDataItem& total, const std::vector<LogLoadDataItem>& data)
{
    beginResetModel();

    data_.clear();

    data_.colLabels_ << "ALL";
    for(size_t i=0; i < total.subReq().size(); i++)
        data_.colLabels_ << "";

    data_.vals_=QVector<QVector<float> >(1+total.subReq().size());
    data_.dataIndex_=QVector<int>(1+total.subReq().size(),-1);

    QVector<float> val;
    for(size_t i=0; i < data.size(); i++)
    {
        val << data[i].sumTotal();
        data_.rowLabels_ << QString::fromStdString(data[i].name());
    }
    data_.vals_[0]=val;
    data_.dataIndex_[0]=-1;

    for(size_t i=0; i < total.subReq().size(); i++)
    {
        size_t pos=0;
        if(columnOrder_ == NameOrder)
            pos=i+1;
        else
            pos=total.subReq()[i].periodStat().rank_+1;

        data_.dataIndex_[pos]=i;
        data_.colLabels_[pos]=QString::fromStdString(total.subReq()[i].name_);

        val.clear();
        for(size_t j=0; j < data.size(); j++)
        {
            val << data[j].subReq()[i].periodStat().sumTotal_;
        }

        data_.vals_[pos]=val;
    }

    endResetModel();
}

void LogStatRequestModel::setDataCmdSuite(const LogLoadDataItem& total, const std::vector<LogLoadDataItem>& data)
{
    beginResetModel();

    data_.clear();

    data_.colLabels_ << "ALL";
    for(size_t i=0; i < data.size(); i++)
        data_.colLabels_ << "";

    data_.vals_=QVector<QVector<float> >(1+data.size());
    data_.dataIndex_=QVector<int>(1+data.size(),-1);

    QVector<float> val;
    for(size_t i=0; i < total.subReq().size(); i++)
    {
        val << total.subReq()[i].periodStat().sumTotal_;
        data_.rowLabels_ << QString::fromStdString(total.subReq()[i].name_);
    }

    data_.vals_[0] =val;
    data_.dataIndex_[0]=-1;

    for(size_t i=0; i < data.size(); i++)
    {
        size_t pos=0;
        if(columnOrder_ == NameOrder)
            pos=i+1;
        else
            pos=data[i].rank()+1;

        data_.dataIndex_[pos]=i;
        data_.colLabels_[pos] = QString::fromStdString(data[i].name());

        val.clear();
        for(size_t j=0; j < data[i].subReq().size(); j++)
        {
            val << data[i].subReq()[j].periodStat().sumTotal_;
        }

        data_.vals_[pos] = val;
    }

    endResetModel();
}

void LogStatRequestModel::setDataSuiteCmd(const LogLoadDataItem& total, const std::vector<LogLoadDataItem>& data)
{
    beginResetModel();

    data_.clear();

    size_t colNum=1+total.subReq().size();

    data_.colLabels_ << "ALL";
    for(size_t i=0; i < colNum-1; i++)
        data_.colLabels_ << "";

    data_.vals_=QVector<QVector<float> >(colNum);
    data_.dataIndex_=QVector<int>(colNum,-1);

    QVector<float> val;
    for(size_t i=0; i < data.size(); i++)
    {
        val << data[i].periodStat().sumTotal_;
        data_.rowLabels_ << QString::fromStdString(data[i].name());
    }

    data_.vals_[0] =val;
    data_.dataIndex_[0]=-1;

    for(size_t i=0; i < total.subReq().size(); i++)
    {
        size_t pos=0;
        if(columnOrder_ == NameOrder)
            pos=i+1;
        else
            pos=total.subReq()[i].periodStat().rank_+1;

        data_.dataIndex_[pos]=i;
        data_.colLabels_[pos] = QString::fromStdString(total.subReq()[i].name_);

        val.clear();
        for(size_t j=0; j < data.size(); j++)
        {
            val << data[j].subReq()[i].periodStat().sumTotal_;
        }

        data_.vals_[pos] = val;
    }

    endResetModel();
}

void LogStatRequestModel::clearData()
{
    beginResetModel();
    data_.clear();
    endResetModel();
}

bool LogStatRequestModel::hasData() const
{
    return !data_.vals_.isEmpty();
}

int LogStatRequestModel::columnCount( const QModelIndex& /*parent */ ) const
{
     return data_.colNum();
}

int LogStatRequestModel::rowCount( const QModelIndex& parent) const
{
    if(!hasData())
        return 0;

    //Parent is the root:
    if(!parent.isValid())
    {
        return data_.rowNum();
    }

    return 0;
}

QVariant LogStatRequestModel::data( const QModelIndex& index, int role ) const
{
    if(!index.isValid() || !hasData())
    {
        return QVariant();
    }
    int row=index.row();
    if(row < 0 || row >= data_.rowNum())
        return QVariant();

    int column=index.column();
    if(column < 0 || column >= data_.colNum())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        return data_.vals_[index.column()][index.row()];
    }
    else if(role == Qt::ForegroundRole)
    {
        if(fabs(data_.vals_[index.column()][index.row()]) < 0.0000001)
            return QColor(190,190,190);
    }
    else if(role == Qt::BackgroundRole)
    {
        if(fabs(data_.vals_[index.column()][index.row()]) > 0.0000001)
            return QColor(192,219,247);
    }

    return QVariant();
}

QVariant LogStatRequestModel::headerData( const int section, const Qt::Orientation orient , const int role ) const
{
    if(orient == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
            return data_.colLabels_[section];
    }
    else if(orient == Qt::Vertical)
    {
        if(role == Qt::DisplayRole)
            return data_.rowLabels_[section];
    }


    return QVariant();
}

QModelIndex LogStatRequestModel::index( int row, int column, const QModelIndex & parent ) const
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

QModelIndex LogStatRequestModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}
