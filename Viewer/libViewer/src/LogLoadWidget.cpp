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
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include "ui_LogLoadWidget.h"

//=======================================================
//
// LogLoadWidget
//
//=======================================================

LogLoadWidget::LogLoadWidget(QWidget *parent) : ui_(new Ui::LogLoadWidget)
{
    ui_->setupUi(this);

    //message label
    ui_->messageLabel->hide();

    //Chart views
    viewHandler_=new LogRequestViewHandler(this);
    for(int i=0; i < viewHandler_->views().count(); i++)
    {
        ui_->viewTab->addTab(viewHandler_->views()[i]," h");
    }

    //Temporal resolution combo box
    ui_->resCombo->addItem("seconds",0);
    ui_->resCombo->addItem("minutes",1);
    ui_->resCombo->addItem("hours",2);

    connect(ui_->resCombo,SIGNAL(currentIndexChanged(int)),
            this,SLOT(resolutionChanged(int)));

    //-----------------------------------------------
    // View + model to display/select suites
    //-----------------------------------------------

    suiteModel_=new LogLoadRequestModel(tr("Suite"),this);
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

    connect(suiteModel_,SIGNAL(checkStateChanged(int,bool)),
            viewHandler_,SLOT(addRemoveSuite(int,bool)));

    connect(viewHandler_,SIGNAL(suitePlotStateChanged(int,bool,QColor)),
            suiteModel_,SLOT(updateItem(int,bool,QColor)));

    connect(ui_->unselectSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(unselectAll()));

    connect(ui_->selectFourSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(selectFirstFourItems()));

    //-----------------------------------------------
    // View + model to display/select child commands
    //-----------------------------------------------

    childReqModel_=new LogLoadRequestModel("Command",this);
    childReqSortModel_=new QSortFilterProxyModel(this);
    childReqSortModel_->setSourceModel(childReqModel_);
    childReqSortModel_->setSortRole(Qt::UserRole);
    childReqSortModel_->setDynamicSortFilter(true);

    ui_->childTree->setRootIsDecorated(false);
    ui_->childTree->setAllColumnsShowFocus(true);
    ui_->childTree->setUniformRowHeights(true);
    ui_->childTree->setSortingEnabled(true);
    ui_->childTree->sortByColumn(1, Qt::DescendingOrder);
    ui_->childTree->setModel(childReqSortModel_);


    connect(childReqModel_,SIGNAL(checkStateChanged(int,bool)),
            viewHandler_,SLOT(addRemoveChildReq(int,bool)));
#if 0
    connect(ui_->loadView,SIGNAL(suitePlotStateChanged(int,bool,QColor)),
            suiteModel_,SLOT(updateSuite(int,bool,QColor)));

    connect(ui_->unselectSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(unselectAllSuites()));

    connect(ui_->selectFourSuitesTb,SIGNAL(clicked()),
            suiteModel_,SLOT(selectFirstFourSuites()));
#endif

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

    connect(viewHandler_, SIGNAL(timeRangeHighlighted(qint64,qint64,qint64)),
            logModel_,SLOT(setHighlightPeriod(qint64,qint64,qint64)));

    connect(viewHandler_,SIGNAL(timeRangeReset()),
            logModel_,SLOT(resetPeriod()));

    //Scan label
    QColor bg(50,52,58);
    ui_->scanLabel->setStyleSheet("QLabel{background: " + bg.name() + ";}");

    QFont font;
    QFontMetrics fm(font);
    int w=fm.width("AAAAAverylongsuitename");
    QList<int> sizeLst=ui_->splitter->sizes();
    if(sizeLst.count()==3)
    {
        sizeLst[2]=w;
        sizeLst[0]=(width()-w)/2;
        sizeLst[1]=(width()-w)/2;
        ui_->splitter->setSizes(sizeLst);
    }

    //logInfo label
    ui_->logInfoLabel->setProperty("fileInfo","1");
    ui_->logInfoLabel->setWordWrap(true);
    ui_->logInfoLabel->setMargin(2);
    ui_->logInfoLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    ui_->logInfoLabel->setAutoFillBackground(true);
    ui_->logInfoLabel->setFrameShape(QFrame::StyledPanel);
    ui_->logInfoLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);


    //Charts
    connect(ui_->showFullTb,SIGNAL(clicked()),
            viewHandler_,SLOT(showFullRange()));

    connect(viewHandler_,SIGNAL(scanDataChanged(QString)),
            ui_->scanLabel,SLOT(setText(QString)));

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
    suiteModel_->clearData();
    childReqModel_->clearData();
    logModel_->clearData();

    setAllVisible(false);
}

void LogLoadWidget::updateInfoLabel()
{
    QColor col(39,49,101);
    QString txt=Viewer::formatBoldText("Log file: ",col) + logFile_;
    txt+=Viewer::formatBoldText(" Server: ",col) + serverName_ +
         Viewer::formatBoldText(" Host: ",col) + host_ +
         Viewer::formatBoldText(" Port: ",col) + port_;

    ui_->logInfoLabel->setText(txt);
}

void LogLoadWidget::setAllVisible(bool b)
{
    ui_->viewTab->setVisible(b);
    ui_->suiteTree->setVisible(b);
    ui_->childTree->setVisible(b);
    ui_->scanLabel->setVisible(b);
    ui_->logView->setVisible(b);
    ui_->unselectSuitesTb->setVisible(b);
    ui_->selectFourSuitesTb->setVisible(b);
}

void LogLoadWidget::load(QString logFile)
{
    load("","","",logFile);
}

void LogLoadWidget::load(QString serverName, QString host, QString port, QString logFile)
{
    clear();

    serverName_=serverName;
    host_=host;
    port_=port;
    logFile_=logFile;

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

    //view_->load("/home/graphics/cgr/ecflow_dev/ecflow-metab.5062.ecf.log");
    //std::string fileName="/home/graphics/cgr/ecflow_dev/vsms1.ecf.log";

    viewHandler_->load(logFile_.toStdString());
    suiteModel_->setData(viewHandler_->data()->suites(),viewHandler_->suitePlotState());
    childReqModel_->setData(viewHandler_->data()->total().childSubReq(),viewHandler_->childPlotState());

    for(int i=0; i < suiteModel_->columnCount()-1; i++)
        ui_->suiteTree->resizeColumnToContents(i);

    logModel_->loadFromFile(logFile_.toStdString());
}

void LogLoadWidget::resolutionChanged(int)
{
    int idx=ui_->resCombo->currentIndex();
    if(idx == 0)
        viewHandler_->setResolution(LogLoadData::SecondResolution);
    else if(idx == 1)
        viewHandler_->setResolution(LogLoadData::MinuteResolution);
}

//=====================================================
//
//  LogLoadSuiteModel
//
//=====================================================

LogLoadSuiteModel::LogLoadSuiteModel(QObject *parent) :
          QAbstractItemModel(parent)
{
}

LogLoadSuiteModel::~LogLoadSuiteModel()
{
}

void LogLoadSuiteModel::setData(LogLoadData* data,QList<bool> checkedLst)
{
    Q_ASSERT(data);
    Q_ASSERT(data->suites().size() == static_cast<size_t>(checkedLst.size()));

    beginResetModel();

    data_.clear();
    for(size_t i=0; i < data->suites().size(); i++)
    {
        data_ << LogLoadSuiteModelDataItem(QString::fromStdString(data->suites()[i].name()),
                                           data->suites()[i].percentage(),checkedLst[i],
                                           data->suites()[i].rank());
    }

    endResetModel();
}


void LogLoadSuiteModel::clearData()
{
    beginResetModel();
    data_.clear();
    endResetModel();
}

bool LogLoadSuiteModel::hasData() const
{
    return !data_.isEmpty();
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
        return data_.count();
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
    if(row < 0 || row >= data_.count())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0:
            return data_[row].suiteName_;
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
            return data_[row].suiteName_;
            break;
        case 1:
            return data_[row].percentage_;
            break;
        default:
            break;
        }
    }
    else if(role == Qt::BackgroundRole)
    {
        return (data_[row].checked_)?data_[row].col_:QVariant();
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
        data_[idx.row()].checked_=checked;
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

void LogLoadSuiteModel::updateSuite(int idx,bool st,QColor col)
{
    if(idx>=0 && idx < data_.size())
    {
        data_[idx].col_=col.lighter(150);
        QModelIndex startIdx=index(idx,0);
        QModelIndex endIdx=index(idx,columnCount()-1);
        Q_EMIT dataChanged(startIdx,endIdx);
    }
}

void LogLoadSuiteModel::unselectAllSuites()
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

void LogLoadSuiteModel::selectFirstFourSuites()
{
    unselectAllSuites();
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

//=====================================================
//
//  LogLoadRequestModel
//
//=====================================================

LogLoadRequestModel::LogLoadRequestModel(QString dataName,QObject *parent) :
          QAbstractItemModel(parent),
          dataName_(dataName)
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
                                           data[i].percentage_,checkedLst[i],
                                           data[i].rank_);
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
                                           data[i].percentage(),checkedLst[i],
                                           data[i].rank());
    }
    endResetModel();
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
    else if(role == Qt::BackgroundRole)
    {
        return (data_[row].checked_)?data_[row].col_:QVariant();
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
    if(perc < 0.5)
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

    views_ << new LogTotalRequestView(this,parent);
    views_ << new LogSuiteRequestView(this,parent);

    for(int i=0; i < views_.count(); i++)
    {
        connect(views_[i],SIGNAL(scanDataChanged(QString)),
                this,SIGNAL(scanDataChanged(QString)));

        connect(views_[i],SIGNAL(suitePlotStateChanged(int,bool,QColor)),
                this,SIGNAL(suitePlotStateChanged(int,bool,QColor)));

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

void LogRequestViewHandler::clear()
{
    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->clear();
    }
}

void LogRequestViewHandler::load(const std::string& logFile)
{
    data_->loadLogFile(logFile);

    suitePlotState_.clear();
    for(size_t i=0; i < data_->suites().size(); i++)
        suitePlotState_ << false;

    childPlotState_.clear();
    for(size_t i=0; i < data_->total().childSubReq().size(); i++)
        childPlotState_ << false;

    userPlotState_.clear();
    for(size_t i=0; i < data_->total().userSubReq().size(); i++)
        userPlotState_ << false;

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

void LogRequestViewHandler::addRemoveChildReq(int idx, bool st)
{
    childPlotState_[idx]=st;

    for(int i=0; i < views_.count(); i++)
    {
        views_[i]->addRemoveChildReq(idx,st);
    }
}

//=============================================
//
// ServerLoadView
//
//=============================================

ServerLoadView::ServerLoadView(QWidget* parent) : QWidget(parent),
    data_(NULL), lastScanIndex_(0)
{
    //The data object - to read and store processed log data
    data_=new LogLoadData();

    QVBoxLayout* vb=new QVBoxLayout(this);
    vb->setContentsMargins(0,0,0,0);

    for(int i=0; i < 3; i++)
    {
        QChart* chart = new QChart();
        ChartView* chartView=new ChartView(chart,this);
        chartView->setRenderHint(QPainter::Antialiasing);
        vb->addWidget(chartView);
        views_ << chartView;

        connect(chartView,SIGNAL(chartZoomed(QRectF)),
                this,SLOT(slotZoom(QRectF)));

        connect(chartView,SIGNAL(positionChanged(qreal)),
                this,SLOT(scanPositionChanged(qreal)));

        connect(chartView,SIGNAL(positionClicked(qreal)),
                this,SLOT(scanPositionClicked(qreal)));
    }

    UI_ASSERT(views_.count() == 3,"views_.count()=" << views_.count());
}

ServerLoadView::~ServerLoadView()
{
    Q_ASSERT(data_);
    delete data_;
}

QChart* ServerLoadView::getChart(ChartType type)
{
    if(ChartView *v=getView(type))
        return v->chart();
    return 0;
}

ChartView* ServerLoadView::getView(ChartType type)
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

void ServerLoadView::slotZoom(QRectF r)
{
    if(ChartView* senderView=static_cast<ChartView*>(sender()))
    {
        Q_FOREACH(ChartView* v,views_)
        {
           if(v != senderView)
               v->doZoom(r);

           v->adjustCallout();
        }

        qint64 start, end;
        Q_ASSERT(!views_.isEmpty());
        views_[0]->currentTimeRange(start,end);
        Q_EMIT timeRangeChanged(start,end);
    }
}

void ServerLoadView::setResolution(LogLoadData::TimeRes res)
{
    data_->setTimeRes(res);

    qint64 start=-1, end=-1;
    if(!views_.isEmpty())
    {
        views_[0]->currentTimeRange(start,end);
    }

    load();
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

void ServerLoadView::addRemoveSuite(int idx, bool st)
{
    if(idx >= 0 && idx < static_cast<int>(data_->suites().size()))
    {
        //Add suite
        if(st)
        {         
            addSuite(idx);
            Q_EMIT suitePlotStateChanged(idx,true,
                   suiteSeriesColour(getChart(TotalChartType),idx));
        }
        //remove
        else
        {
            suitePlotState_[idx]=false;

            removeSuiteSeries(getChart(TotalChartType),
                              "s_main_" + QString::number(idx));

            removeSuiteSeries(getChart(ChildChartType),
                              "s_ch_" + QString::number(idx));

            removeSuiteSeries(getChart(UserChartType),
                              "s_us_" + QString::number(idx));
        }
    }
}

void ServerLoadView::addSuite(int idx)
{
    suitePlotState_[idx]=true;

    QChart* chart=0;

    QLineSeries* series=new QLineSeries();
    series->setName("s_main_" + QString::number(idx));
    data_->getSuiteTotalReq(idx,*series);
    chart=getChart(TotalChartType);
    chart->addSeries(series);
    series->attachAxis(chart->axisX());
    series->attachAxis(chart->axisY());

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("s_ch_" + QString::number(idx));
    data_->getSuiteChildReq(idx,*chSeries);
    chart=getChart(ChildChartType);
    chart->addSeries(chSeries);
    chSeries->attachAxis(chart->axisX());
    chSeries->attachAxis(chart->axisY());

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("s_us_" + QString::number(idx));
    data_->getSuiteUserReq(idx,*usSeries);
    chart=getChart(UserChartType);
    chart->addSeries(usSeries);
    usSeries->attachAxis(chart->axisX());
    usSeries->attachAxis(chart->axisY());
}

void ServerLoadView::removeSuiteSeries(QChart* chart,QString id)
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

QColor ServerLoadView::suiteSeriesColour(QChart* chart,size_t idx)
{
    QString id="_" + QString::number(idx);
    return seriesColour(chart,id);
}

QColor ServerLoadView::seriesColour(QChart* chart,QString id)
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

void ServerLoadView::clear()
{
    clearCharts();
    data_->clear();
    suitePlotState_.clear();
    size_t lastScanIndex_=0;;
}

void ServerLoadView::clearCharts()
{
    Q_FOREACH(ChartView* v,views_)
    {
        Q_ASSERT(v->chart());
        v->chart()->removeAllSeries();
        v->chart()->removeAxis(v->chart()->axisX());
        v->chart()->removeAxis(v->chart()->axisY());
    }
}

void ServerLoadView::load(const std::string& logFile)
{
    data_->loadLogFile(logFile);

    load();
    loadSuites();

    suitePlotState_.clear();
    for(size_t i=0; i < data_->suites().size(); i++)
        suitePlotState_ << false;

}

void ServerLoadView::load()
{
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

void ServerLoadView::loadSuites()
{
    for(int i=0; i < suitePlotState_.count(); i++)
    {
        if(suitePlotState_[i])
        {
            addSuite(i);
        }
    }
}

void  ServerLoadView::build(ChartView* view,QLineSeries *series, QString title,int maxVal)
{
    Q_ASSERT(view);
    QChart *chart=view->chart();
    Q_ASSERT(chart);

    chart->addSeries(series);
    chart->setTitle(title);

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

    axisY->setTitleText(yTitle);
    axisY->setMin(0.);
    chart->setAxisY(axisY, series);
    axisY->setMin(0.);
    axisY->setMax(maxVal);
}

void ServerLoadView::showFullRange()
{
    Q_FOREACH(ChartView* view,views_)
        view->chart()->zoomReset();

    Q_EMIT(timeRangeReset());
}

void ServerLoadView::scanPositionClicked(qreal pos)
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
        if(data_->timeRes() == LogLoadData::MinuteResolution)
            t2=t1+60*1000;

        Q_FOREACH(ChartView* view,views_)
            view->setCallout(pos);

        qint64 tw=0;
        if(!views_.isEmpty())
            tw=views_[0]->widthToTimeRange(50.);

        Q_EMIT(timeRangeHighlighted(t1,t2,tw));
    }
}


void ServerLoadView::scanPositionChanged(qreal pos)
{
    qint64 t(pos);
    size_t idx=0;

    if(views_.isEmpty())
        return;

    qint64 tw=views_[0]->widthToTimeRange(50.);

    bool hasData=data_->indexOfTime(t,idx,lastScanIndex_,tw);

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

    dateTxt=(hasData)?QDateTime::fromMSecsSinceEpoch(data_->time()[idx]).toString("hh:mm:ss dd/MM/yyyy"):" N/A";
    txt+="<tr>" +
        Viewer::formatTableTdText("Date (nearest):",dateCol) +
        Viewer::formatTableTdBg(dateTxt,dateCol) +
        "</tr>";
    txt+="</table>";


   // txt+="<br>" + Viewer::formatText("date (data): " + dateTxt, dateCol);

    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";
    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Item",hdrCol) +
              Viewer::formatTableThText("Total",hdrCol) +
              Viewer::formatTableThText("Child",hdrCol) +
              Viewer::formatTableThText("User",hdrCol) + "</tr>";


    if(hasData)
    {
        size_t tot=0,ch=0,us=0;
        QColor col=seriesColour(getChart(TotalChartType),"all");
        QString name="all";
        data_->dataItem().valuesAt(idx,tot,ch,us);
        buildScanRow(txt,name,tot,ch,us,col);

        for(int i=0; i < suitePlotState_.size(); i++)
        {
            if(suitePlotState_[i])
            {
                tot=0;ch=0;us=0;
                col=suiteSeriesColour(getChart(TotalChartType),i);
                name=QString::fromStdString(data_->suites()[i].name());
                data_->suites()[i].valuesAt(idx,tot,ch,us);
                buildScanRow(txt,name,tot,ch,us,col);
            }
        }
    }
    else
    {
        QColor col=seriesColour(getChart(TotalChartType),"all");
        QString name="all";
        buildEmptyScanRow(txt,name,col);
        for(int i=0; i < suitePlotState_.size(); i++)
        {
            if(suitePlotState_[i])
            {
                col=suiteSeriesColour(getChart(TotalChartType),i);
                name=QString::fromStdString(data_->suites()[i].name());
                buildEmptyScanRow(txt,name,col);
            }
        }
    }

    txt+="</table>";

    Q_EMIT scanDataChanged(txt);
}

void ServerLoadView::buildScanRow(QString &txt,QString name,size_t tot,size_t ch,size_t us,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(QString::number(tot),numBg) +
          Viewer::formatTableTdBg(QString::number(ch),numBg) +
          Viewer::formatTableTdBg(QString::number(us),numBg) + "</tr>";
}

void ServerLoadView::buildEmptyScanRow(QString &txt,QString name,QColor lineCol) const
{
    QColor numBg(210,211,214);
    txt+="<tr>" + Viewer::formatTableTdBg(name,lineCol.lighter(150)) +
          Viewer::formatTableTdBg(" N/A",numBg) +
          Viewer::formatTableTdBg(" N/A",numBg) +
          Viewer::formatTableTdBg(" N/A",numBg) + "</tr>";
}

//=============================================
//
// ServerLoadView
//
//=============================================

LogRequestView::LogRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    QWidget(parent),
    handler_(handler)
{
    mainLayout_=new QVBoxLayout(this);
    mainLayout_->setContentsMargins(0,0,0,0);
}

LogRequestView::~LogRequestView()
{
}

QChart* LogRequestView::addChartById(int id)
{
    QChart* chart = new QChart();
    ChartView* chartView=new ChartView(chart,this);
    chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout_->addWidget(chartView);
    views_ << chartView;

    connect(chartView,SIGNAL(chartZoomed(QRectF)),
            this,SLOT(slotZoom(QRectF)));

    connect(chartView,SIGNAL(positionChanged(qreal)),
            this,SLOT(scanPositionChanged(qreal)));

    connect(chartView,SIGNAL(positionClicked(qreal)),
            this,SLOT(scanPositionClicked(qreal)));

    viewIds_[id]=chartView;
    return chart;
}

void LogRequestView::removeChartById(int id)
{
    if(ChartView* chartView=viewIds_.value(id,NULL))
    {
        mainLayout_->removeWidget(chartView);
        delete chartView;
        views_.removeOne(chartView);
    }

}

#if 0
QChart* ServerLoadView::getChart(ChartType type)
{
    if(ChartView *v=getView(type))
        return v->chart();
    return 0;
}

ChartView* ServerLoadView::getView(ChartType type)
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

#endif

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

        qint64 start, end;
        Q_ASSERT(!views_.isEmpty());
        views_[0]->currentTimeRange(start,end);
        Q_EMIT timeRangeChanged(start,end);
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

#if 0
void LogRequestView::addRemoveSuite(int idx, bool st)
{
    if(idx >= 0 && idx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addSuite(idx);
            Q_EMIT suitePlotStateChanged(idx,true,
                   suiteSeriesColour(getChart(TotalChartType),idx));
        }
        //remove
        else
        {
            //suitePlotState_[idx]=false;

            removeSuiteSeries(getChart(TotalChartType),
                              "s_main_" + QString::number(idx));

            removeSuiteSeries(getChart(ChildChartType),
                              "s_ch_" + QString::number(idx));

            removeSuiteSeries(getChart(UserChartType),
                              "s_us_" + QString::number(idx));
        }
    }
}
#endif

#if 0
void LogRequestView::addSuite(int idx)
{
    //suitePlotState_[idx]=true;

    QChart* chart=0;

    QLineSeries* series=new QLineSeries();
    series->setName("s_main_" + QString::number(idx));
    data_->getSuiteTotalReq(idx,*series);
    chart=getChart(TotalChartType);
    chart->addSeries(series);
    series->attachAxis(chart->axisX());
    series->attachAxis(chart->axisY());

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("s_ch_" + QString::number(idx));
    data_->getSuiteChildReq(idx,*chSeries);
    chart=getChart(ChildChartType);
    chart->addSeries(chSeries);
    chSeries->attachAxis(chart->axisX());
    chSeries->attachAxis(chart->axisY());

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("s_us_" + QString::number(idx));
    data_->getSuiteUserReq(idx,*usSeries);
    chart=getChart(UserChartType);
    chart->addSeries(usSeries);
    usSeries->attachAxis(chart->axisX());
    usSeries->attachAxis(chart->axisY());
}
#endif

void LogRequestView::removeSuiteSeries(QChart* chart,QString id)
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

QColor LogRequestView::suiteSeriesColour(QChart* chart,size_t idx)
{
    QString id="_" + QString::number(idx);
    return seriesColour(chart,id);
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
    //suitePlotState_.clear();
    //size_t lastScanIndex_=0;;
}

void LogRequestView::clearCharts()
{
    Q_FOREACH(ChartView* v,views_)
    {
        Q_ASSERT(v->chart());
        v->chart()->removeAllSeries();
        v->chart()->removeAxis(v->chart()->axisX());
        v->chart()->removeAxis(v->chart()->axisY());
    }
}

void LogRequestView::load()
{
    //data_->loadLogFile(logFile);

    loadCore();
    loadSuites();

    //suitePlotState_.clear();
    //for(size_t i=0; i < data_->suites().size(); i++)
    //    suitePlotState_ << false;
}

#if 0
void LogRequestView::loadCore()
{
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
#endif

void LogRequestView::loadSuites()
{
    for(int i=0; i < handler_->suitePlotState_.count(); i++)
    {
        if(handler_->suitePlotState_[i])
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

    chart->legend()->hide();
    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("HH dd/MM");
    chart->setAxisX(axisX, series);
    view->adjustTimeAxis(handler_->data_->period());

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");

    QString yTitle;
    if(handler_->data_->timeRes() == LogLoadData::SecondResolution)
        yTitle="Req. per second";
    else if(handler_->data_->timeRes() == LogLoadData::MinuteResolution)
        yTitle="Req. per minute";

    axisY->setTitleText(yTitle);
    axisY->setMin(0.);
    chart->setAxisY(axisY, series);
    axisY->setMin(0.);
    axisY->setMax(maxVal);
}

void LogRequestView::showFullRange()
{
    Q_FOREACH(ChartView* view,views_)
        view->chart()->zoomReset();

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
        if(handler_->data_->timeRes() == LogLoadData::MinuteResolution)
            t2=t1+60*1000;

        Q_FOREACH(ChartView* view,views_)
            view->setCallout(pos);

        qint64 tw=0;
        if(!views_.isEmpty())
            tw=views_[0]->widthToTimeRange(50.);

        Q_EMIT(timeRangeHighlighted(t1,t2,tw));
    }
}


void LogRequestView::scanPositionChanged(qreal pos)
{
    qint64 t(pos);
    size_t idx=0;

    if(views_.isEmpty())
        return;

    qint64 tw=views_[0]->widthToTimeRange(50.);

    bool hasData=handler_->data_->indexOfTime(t,idx,handler_->lastScanIndex_,tw);

    handler_->lastScanIndex_=idx;
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

    dateTxt=(hasData)?QDateTime::fromMSecsSinceEpoch(handler_->data_->time()[idx]).toString("hh:mm:ss dd/MM/yyyy"):" N/A";
    txt+="<tr>" +
        Viewer::formatTableTdText("Date (nearest):",dateCol) +
        Viewer::formatTableTdBg(dateTxt,dateCol) +
        "</tr>";

#if 0
    txt+="</table>";


   // txt+="<br>" + Viewer::formatText("date (data): " + dateTxt, dateCol);

    txt+="<br><table width=\'100%\' cellpadding=\'4px\'>";
    //header
    QColor hdrCol(205,206,210);
    txt+="<tr>" + Viewer::formatTableThText("Item",hdrCol) +
              Viewer::formatTableThText("Total",hdrCol) +
              Viewer::formatTableThText("Child",hdrCol) +
              Viewer::formatTableThText("User",hdrCol) + "</tr>";


    if(hasData)
    {
        size_t tot=0,ch=0,us=0;
        QColor col=seriesColour(getChart(TotalChartType),"all");
        QString name="all";
        data_->dataItem().valuesAt(idx,tot,ch,us);
        buildScanRow(txt,name,tot,ch,us,col);

        for(int i=0; i < suitePlotState_.size(); i++)
        {
            if(suitePlotState_[i])
            {
                tot=0;ch=0;us=0;
                col=suiteSeriesColour(getChart(TotalChartType),i);
                name=QString::fromStdString(handler_->data_->suites()[i].name());
                data_->suites()[i].valuesAt(idx,tot,ch,us);
                buildScanRow(txt,name,tot,ch,us,col);
            }
        }
    }
    else
    {
        QColor col=seriesColour(getChart(TotalChartType),"all");
        QString name="all";
        buildEmptyScanRow(txt,name,col);
        for(int i=0; i < suitePlotState_.size(); i++)
        {
            if(suitePlotState_[i])
            {
                col=suiteSeriesColour(getChart(TotalChartType),i);
                name=QString::fromStdString(handler_->data_->suites()[i].name());
                buildEmptyScanRow(txt,name,col);
            }
        }
    }

    txt+="</table>";
#endif

    Q_EMIT scanDataChanged(txt);
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

//=============================================================================
//
// LogTotalRequestView
//
//=============================================================================

LogTotalRequestView::LogTotalRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);

    for(int i=0; i < 3; i++)
    {
        addChartById(i);

#if 0
        QChart* chart = new QChart();
        ChartView* chartView=new ChartView(chart,this);
        chartView->setRenderHint(QPainter::Antialiasing);
        mainLayout_->addWidget(chartView);
        views_ << chartView;

        connect(chartView,SIGNAL(chartZoomed(QRectF)),
                this,SLOT(slotZoom(QRectF)));

        connect(chartView,SIGNAL(positionChanged(qreal)),
                this,SLOT(scanPositionChanged(qreal)));

        connect(chartView,SIGNAL(positionClicked(qreal)),
                this,SLOT(scanPositionClicked(qreal)));
#endif

    }

    UI_ASSERT(views_.count() == 3,"views_.count()=" << views_.count());
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
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addSuite(suiteIdx);
            Q_EMIT suitePlotStateChanged(suiteIdx,true,
                   suiteSeriesColour(getChart(TotalChartType),suiteIdx));
        }
        //remove
        else
        {
            //suitePlotState_[idx]=false;

            removeSuiteSeries(getChart(TotalChartType),
                              "s_main_" + QString::number(suiteIdx));

            removeSuiteSeries(getChart(ChildChartType),
                              "s_ch_" + QString::number(suiteIdx));

            removeSuiteSeries(getChart(UserChartType),
                              "s_us_" + QString::number(suiteIdx));
        }
    }
}

void LogTotalRequestView::addSuite(int idx)
{
    //suitePlotState_[idx]=true;

    QChart* chart=0;

    QLineSeries* series=new QLineSeries();
    series->setName("s_main_" + QString::number(idx));
    handler_->data_->getSuiteTotalReq(idx,*series);
    chart=getChart(TotalChartType);
    chart->addSeries(series);
    series->attachAxis(chart->axisX());
    series->attachAxis(chart->axisY());

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("s_ch_" + QString::number(idx));
    handler_->data_->getSuiteChildReq(idx,*chSeries);
    chart=getChart(ChildChartType);
    chart->addSeries(chSeries);
    chSeries->attachAxis(chart->axisX());
    chSeries->attachAxis(chart->axisY());

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("s_us_" + QString::number(idx));
    handler_->data_->getSuiteUserReq(idx,*usSeries);
    chart=getChart(UserChartType);
    chart->addSeries(usSeries);
    usSeries->attachAxis(chart->axisX());
    usSeries->attachAxis(chart->axisY());
}

void LogTotalRequestView::loadCore()
{
    clearCharts();

    int maxVal=0;
    QLineSeries* tSeries=new QLineSeries();
    tSeries->setName("all");
    handler_->data_->getTotalReq(*tSeries,maxVal);

    QLineSeries* chSeries=new QLineSeries();
    chSeries->setName("all");
    handler_->data_->getChildReq(*chSeries);

    QLineSeries* usSeries=new QLineSeries();
    usSeries->setName("all");
    handler_->data_->getUserReq(*usSeries);

    build(getView(TotalChartType),tSeries,"Child+User requests",maxVal);
    build(getView(ChildChartType),chSeries,"Child requests",maxVal);
    build(getView(UserChartType),usSeries,"User requests",maxVal);
}

//=============================================================================
//
// LogSuiteRequestView
//
//=============================================================================

LogSuiteRequestView::LogSuiteRequestView(LogRequestViewHandler* handler,QWidget* parent) :
    LogRequestView(handler,parent)
{
    Q_ASSERT(mainLayout_);
}

//One chart = one suite
void LogSuiteRequestView::addRemoveSuite(int suiteIdx, bool st)
{
    if(suiteIdx >= 0 && suiteIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addSuite(suiteIdx);
        }
        //remove
        else
        {
            removeChartById(suiteIdx);
        }
    }
}

void LogSuiteRequestView::addSuite(int suiteIdx)
{
    QChart* chart=addChartById(suiteIdx);

    for(int i=0; i < handler_->childPlotState_.count(); i++)
    {
        if(handler_->childPlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName("c_" + QString::number(suiteIdx) + "_" + QString::number(i));
            handler_->data_->getSuiteChildSubReq(suiteIdx,i,*series);
            chart->addSeries(series);
            series->attachAxis(chart->axisX());
            series->attachAxis(chart->axisY());
        }
    }

    for(int i=0; i < handler_->userPlotState_.count(); i++)
    {
        if(handler_->userPlotState_[i])
        {
            QLineSeries* series=new QLineSeries();
            series->setName("u_" + QString::number(suiteIdx) + "_" + QString::number(i));
            handler_->data_->getSuiteUserSubReq(suiteIdx,i,*series);
            chart->addSeries(series);
            series->attachAxis(chart->axisX());
            series->attachAxis(chart->axisY());
        }
    }
}

//One chart = one suite
void LogSuiteRequestView::addRemoveChildReq(int childReqIdx, bool st)
{
    if(childReqIdx >= 0 && childReqIdx < static_cast<int>(handler_->data_->suites().size()))
    {
        //Add suite
        if(st)
        {
            addChildReq(childReqIdx);
        }
        //remove
        else
        {
            //removeChartById(suiteIdx);
        }
    }
}

void LogSuiteRequestView::addChildReq(int childReqIdx)
{
    for(int i=0; i < views_.count(); i++)
    {
        int suiteIdx=chartId(views_[i]);
        Q_ASSERT(suiteIdx >= 0);

        QLineSeries* series=new QLineSeries();
        series->setName("c_" + QString::number(suiteIdx) + "_" + QString::number(i));
        handler_->data_->getSuiteChildSubReq(suiteIdx,childReqIdx,*series);
        QChart* chart=views_[i]->chart();
        chart->addSeries(series);
        series->attachAxis(chart->axisX());
        series->attachAxis(chart->axisY());
    }
}

int  LogSuiteRequestView::chartId(ChartView* cv)
{
    return viewIds_.key(cv,-1);
}

void LogSuiteRequestView::loadCore()
{

}
